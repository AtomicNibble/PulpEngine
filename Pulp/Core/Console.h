#pragma once

#ifndef _X_CONSOLE_DEF_H_
#define _X_CONSOLE_DEF_H_

#include <IConsole.h>
#include <IInput.h>
#include <ITexture.h>
#include <IDirectoryWatcher.h>

X_DISABLE_WARNING(4702)
#include <deque>
X_ENABLE_WARNING(4702)

#include <String\StrRef.h>

#include <Time\TimeVal.h>
#include <Util\UniquePointer.h>

#include "Logging\Logger.h"
#include "Logging\FilterPolicies\LoggerNoFilterPolicy.h"
#include "Logging\FormatPolicies\LoggerInternalConsoleFormatPolicy.h"
#include "Logging\WritePolicies\LoggerInternalConsoleWritePolicy.h"

#include <Containers\FixedHashTable.h>
#include <Containers\Fifo.h>

#include <Memory\AllocationPolicies\PoolAllocator.h>

X_NAMESPACE_DECLARE(engine,
                    class IPrimativeContext);

X_NAMESPACE_BEGIN(core)

struct IoRequestBase;
struct XFileAsync;

typedef core::MemoryArena<
    core::PoolAllocator,
    core::SingleThreadPolicy,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
    core::SimpleBoundsChecking,
    core::SimpleMemoryTracking,
    core::SimpleMemoryTagging
#else
    core::NoBoundsChecking,
    core::NoMemoryTracking,
    core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_DEBUG_POLICIES
    >
    VarPool;

struct ConsoleCommand
{
    typedef VarFlags VarFlags;

    ConsoleCommand();

    string name; // the name of the command.
    string desc; // descption text for the command

    VarFlags flags;      // flags like CHEAT etc..
    ConsoleCmdFunc func; // Pointer to console command.
};

struct ConsoleCommandArgs : public IConsoleCmdArgs
{
    X_DECLARE_FLAGS(ParseFlag)(
        SINGLE_ARG    
    );

    using ParseFlags = Flags<ParseFlag>;

    static const size_t MAX_COMMAND_NAME_LEN = 64;
    static const size_t MAX_STRING_CHARS = 1024;
    static const size_t MAX_COMMAND_ARGS = 64;
    static const size_t MAX_COMMAND_STRING = 2 * MAX_STRING_CHARS;

    using CommandNameStr = core::StackString<MAX_COMMAND_NAME_LEN>;
    using CommandStr = core::StackString<ConsoleCommandArgs::MAX_STRING_CHARS>;

    using IndexRange = std::pair<int32_t, int32_t>;
    using IndexRangeArr = std::array<IndexRange, MAX_COMMAND_ARGS>;
    using StringBufArr = std::array<char, MAX_COMMAND_STRING>;

public:
    explicit ConsoleCommandArgs(const CommandStr& line, ParseFlags flags);
    ~ConsoleCommandArgs() X_FINAL;

    virtual size_t GetArgCount(void) const X_FINAL;
    virtual core::string_view GetArg(size_t idx) const X_FINAL;
    virtual core::string_view GetArgToEnd(size_t idx) const X_FINAL;

private:
    void TokenizeString(ParseFlags flags);

private:
    size_t argNum_;                 // number of arguments
    IndexRangeArr argv_;            // offset + length of each arg.
    const CommandStr str_;
};

X_DECLARE_ENUM(CmdHistory)
(
    UP,
    DOWN);

X_DECLARE_FLAGS(ExecSource)
(
    CONSOLE,
    CONFIG,
    SYSTEM);

class XConsole : public IConsole
    , public ICoreEventListener
{
    struct ExecCommand
    {
        ExecCommand(const string& command, ExecSource::Enum src, bool silentMode);
        ExecCommand() = default;

        string command;
        ExecSource::Enum source;
        bool silentMode;
    };

    struct Cursor
    {
        Cursor();

        TimeVal curTime;
        TimeVal displayTime;
        bool draw;
        bool _pad[3];
    };

    
    // key points into string stored in ICVar or in .exe/.dll
    typedef core::FixedHashTable<core::string_view, ICVar*, core::hash<core::string_view>, equal_to_case_insen<core::string_view>> ConsoleVarMap; 
    typedef core::FixedHashTable<string, ConsoleCommand, core::hash<string>, equal_to_case_insen<core::string>> ConsoleCmdMap;
    typedef core::FixedHashTable<string, string, core::hash<string>, equal_to_case_insen<core::string>> ConfigCmdsMap;
    typedef core::FixedHashTable<string, string> ConsoleBindMap;

    typedef core::Fifo<ExecCommand> ExecCmdList;

    typedef core::Fifo<string> ConsoleBuffer;

    typedef core::Logger<
        core::LoggerNoFilterPolicy,
        core::LoggerInternalConsoleFormatPolicy,
        core::LoggerInternalConsoleWritePolicy>
        Logger;

public:
    static const size_t MAX_HISTORY_ENTRIES = 64;

    static const char* CMD_HISTORY_FILE_NAME;
    static const char* USER_CFG_FILE_NAME;

    typedef consoleState consoleState;

public:
    XConsole();

    ~XConsole() X_FINAL;

    void registerVars(void) X_FINAL;
    void registerCmds(void) X_FINAL;

    // called at start when not much else exists, just so subsystems can register vars
    bool init(ICore* pCore, bool basic) X_FINAL;
    // finialize any async init tasks.
    bool asyncInitFinalize(void) X_FINAL;
    bool loadRenderResources(void) X_FINAL;

    void shutDown(void) X_FINAL;
    void freeRenderResources(void) X_FINAL;
    void saveChangedVars(void) X_FINAL; // saves vars with 'SAVE_IF_CHANGED' if modified.

    void dispatchRepeateInputEvents(core::FrameTimeData& time) X_FINAL;
    void runCmds(void) X_FINAL;
    void draw(core::FrameTimeData& time) X_FINAL;

    X_INLINE consoleState::Enum getVisState(void) const X_FINAL;

    // input callbacks
    bool onInputEvent(const input::InputEvent& event);

    ICVar* registerString(core::string_view name, core::string_view value, VarFlags flags, core::string_view desc) X_FINAL;
    ICVar* registerInt(core::string_view name, int Value, int Min, int Max, VarFlags flags, core::string_view desc) X_FINAL;
    ICVar* registerFloat(core::string_view name, float Value, float Min, float Max, VarFlags flags, core::string_view desc) X_FINAL;

    // refrenced based, these are useful if we want to use the value alot so we just register it's address.
    ICVar* registerRef(core::string_view name, float* src, float defaultvalue, float Min, float Max, VarFlags flags, core::string_view desc) X_FINAL;
    ICVar* registerRef(core::string_view name, int* src, int defaultvalue, int Min, int Max, VarFlags flags, core::string_view desc) X_FINAL;
    ICVar* registerRef(core::string_view name, Color* src, Color defaultvalue, VarFlags flags, core::string_view desc) X_FINAL;
    ICVar* registerRef(core::string_view name, Vec3f* src, Vec3f defaultvalue, VarFlags flags, core::string_view desc) X_FINAL;

    ICVar* getCVar(core::string_view name) X_FINAL;

    void unregisterVariable(core::string_view varName) X_FINAL;
    void unregisterVariable(ICVar* pVar) X_FINAL;

    void registerCommand(core::string_view name, ConsoleCmdFunc func, VarFlags Flags, core::string_view desc) X_FINAL;
    void unRegisterCommand(core::string_view name) X_FINAL;

    void exec(core::string_view command) X_FINAL;
    bool loadAndExecConfigFile(core::string_view fileName) X_FINAL;

    void addLineToLog(const char* pStr, uint32_t length) X_FINAL;
    int32_t getLineCount(void) const X_FINAL;

    X_INLINE void showConsole(consoleState::Enum state);
    X_INLINE bool isVisable(void) const;
    X_INLINE bool isExpanded(void) const;
    X_INLINE void toggleConsole(bool expand);

private:
    // vars
    ICVar* getCVarForRegistration(core::string_view name);
    void registerVar(ICVar* pCVar);

    void displayVarValue(const ICVar* pVar);
    void displayVarInfo(const ICVar* pVar, bool full = false);

    // Cmds
    void addCmd(core::string_view command, ExecSource::Enum src, bool silent);
    void addCmd(string&& command, ExecSource::Enum src, bool silent);
    void executeStringInternal(const ExecCommand& cmd); // executes a command string, may contain multiple commands
                                                        // runs a single cmd. even tho it's 'const' a command may alter the consoles state. :| aka 'clearBinds', 'consoleHide', ...
    void executeCommand(const ConsoleCommand& cmd, ConsoleCommandArgs::CommandStr& str) const;

    // Config
    void configExec(const char* pCommand, const char* pEnd);

    // input
    void addInputChar(const char c);
    void removeInputChar(bool bBackSpace);
    void clearInputBuffer(void);
    void executeInputBuffer(void);

    bool handleInput(const input::InputEvent& event);
    bool handleInputChar(const input::InputEvent& event);
    bool processInput(const input::InputEvent& event);

    // History
    void saveCmdHistory(void) const;
    void loadCmdHistory(bool async);
    void historyIoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
        core::XFileAsync* pFile, uint32_t bytesTransferred);
    void parseCmdHistory(const char* pBegin, const char* pEnd);
    void addCmdToHistory(const char* pCommand);
    void addCmdToHistory(const string& command);
    void resetHistoryPos(void);
    core::string_view getHistory(CmdHistory::Enum direction);

    // Binds 
    void addBind(core::string_view key, core::string_view cmd);
    core::string_view findBind(core::string_view key);
    void clearAllBinds(void);
    void listbinds(IKeyBindDumpSink* CallBack);

private:
    // scroll helpers
    void pageUp(void);
    void pageDown(void);
    void validateScrollPos(void);

private:
    // AutoComplete
    void resetAutoCompletion(void);

    X_INLINE bool isAutocompleteVis(void);

    // returns the max log lines that fit on screen.
    int32_t maxVisibleLogLines(void) const;

private:
    void drawBuffer(void);
    void drawScrollBar(void);
    void drawInputTxt(const Vec2f& start);

    void copy(void);
    void paste(void);

private:
    // ICoreEventListener
    void OnCoreEvent(const CoreEventData& ed) X_FINAL;
    // ~ICoreEventListener

private:
    void listCommands(core::string_view searchPattern);
    void listVariables(core::string_view searchPattern);
    void listVariablesValues(core::string_view searchPattern);

private:
    void Command_Exit(IConsoleCmdArgs* Cmd);
    void Command_Exec(IConsoleCmdArgs* Cmd);
    void Command_History(IConsoleCmdArgs* Cmd);
    void Command_Help(IConsoleCmdArgs* Cmd);
    void Command_ListCmd(IConsoleCmdArgs* Cmd);
    void Command_ListDvars(IConsoleCmdArgs* Cmd);
    void Command_ListDvarsValues(IConsoleCmdArgs* Cmd);
    void Command_Echo(IConsoleCmdArgs* Cmd);
    void Command_VarReset(IConsoleCmdArgs* Cmd);
    void Command_VarDescribe(IConsoleCmdArgs* Cmd);
    void Command_Bind(IConsoleCmdArgs* Cmd);
    void Command_BindsClear(IConsoleCmdArgs* Cmd);
    void Command_BindsList(IConsoleCmdArgs* Cmd);
    void Command_SetVarArchive(IConsoleCmdArgs* Cmd);
    void Command_ConsoleShow(IConsoleCmdArgs* Cmd);
    void Command_ConsoleHide(IConsoleCmdArgs* Cmd);
    void Command_ConsoleToggle(IConsoleCmdArgs* Cmd);
    void Command_SaveModifiedVars(IConsoleCmdArgs* Cmd);

private:
    core::HeapArea varHeap_;
    core::PoolAllocator varAllocator_;
    VarPool varArena_;

    Logger logger_;

    ConsoleBuffer cmdHistory_;
    // some sort of lock free ring buffer might work better for this log.
    ConsoleBuffer consoleLog_;
    core::SpinlockRecursive logLock_;

    ConsoleVarMap varMap_;
    ConsoleCmdMap cmdMap_;
    ConsoleBindMap binds_; // support sexy bind.
    ConfigCmdsMap configCmds_;
    ConfigCmdsMap varArchive_;

    ExecCmdList pendingCmdsQueue_;

    consoleState::Enum consoleState_;

    int32_t historyPos_;
    int32_t cursorPos_;
    int32_t scrollPos_;
    string inputBuffer_;
    string refString_;

    Vec2<int32_t> renderRes_;

    X_DISABLE_WARNING(4324);
    input::InputEvent repeatEvent_;
    X_ENABLE_WARNING(4324);

    TimeVal repeatEventInterval_;
    TimeVal repeatEventInitialDelay_;
    TimeVal repeatEventTimer_; // the time a repeat event will be trigger.

    ICore* pCore_;
    font::IFont* pFont_;
    render::IRender* pRender_;
    engine::IPrimativeContext* pPrimContext_;

    Cursor cursor_;

    bool coreEventListernRegd_;

    // async history loading
    bool historyLoadPending_;
    uint32_t historyFileSize_;
    core::UniquePointer<const char[]> historyFileBuf_;
    core::CriticalSection historyFileLock_;
    core::ConditionVariable historyFileCond_;

    // auto complete
    int32_t autoCompleteNum_;
    int32_t autoCompleteIdx_;
    bool autoCompleteSelect_;

private:
    int console_debug;
    int console_case_sensitive;
    int console_save_history;
    int console_buffer_size;
    int console_output_draw_channel;
    int console_cursor_skip_color_codes;
    int console_disable_mouse;
    // text
    int console_output_font_size;
    float console_output_font_line_height;

    Color console_cmd_color;
    Color console_input_box_color;
    Color console_input_box_color_border;
    Color console_output_box_color;
    Color console_output_box_color_border;
    Color console_output_box_channel_color;
    Color console_output_scroll_bar_color;
    Color console_output_scroll_bar_slider_color;
};

X_NAMESPACE_END

#include "Console.inl"

#endif // _X_CONSOLE_DEF_H_
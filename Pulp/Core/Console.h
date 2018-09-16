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

#include <Containers\HashMap.h>
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

struct equal_to_case_insen
{
    bool operator()(const char* const _Left, const char* const _Right) const
    {
        return core::strUtil::IsEqualCaseInsen(_Left, _Right);
    }
};

struct ConsoleCommand
{
    ConsoleCommand();

    typedef VarFlags FlagType;

    string Name; // the name of the command.
    string Desc; // descption text for the command

    FlagType Flags;      // flags like CHEAT etc..
    ConsoleCmdFunc func; // Pointer to console command.
};

struct ConsoleCommandArgs : public IConsoleCmdArgs
{
    static const size_t MAX_STRING_CHARS = 1024;
    static const size_t MAX_COMMAND_ARGS = 64;
    static const size_t MAX_COMMAND_STRING = 2 * MAX_STRING_CHARS;

public:
    explicit ConsoleCommandArgs(core::StackString<ConsoleCommandArgs::MAX_STRING_CHARS>& line);
    ~ConsoleCommandArgs() X_OVERRIDE;

    virtual size_t GetArgCount(void) const X_OVERRIDE;
    virtual const char* GetArg(size_t idx) const X_OVERRIDE;
    void TokenizeString(const char* begin, const char* end);

private:
    size_t argNum_;                      // number of arguments
    char* argv_[MAX_COMMAND_ARGS];       // points into tokenized
    char tokenized_[MAX_COMMAND_STRING]; // will have terminator bytes inserted
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

    // members.
    typedef core::HashMap<const char*, ICVar*, core::hash<const char*>, equal_to_case_insen> ConsoleVarMap; // key points into string stored in ICVar or in .exe/.dll
    typedef core::HashMap<string, ConsoleCommand, core::hash<const char*>, equal_to_case_insen> ConsoleCmdMap;
    typedef core::HashMap<string, string, core::hash<const char*>, equal_to_case_insen> ConfigCmdsMap;
    typedef core::HashMap<string, string> ConsoleBindMap;

    typedef core::Fifo<ExecCommand> ExecCmdList;

    typedef core::Fifo<string> ConsoleBuffer;

    typedef core::Logger<
        core::LoggerNoFilterPolicy,
        core::LoggerInternalConsoleFormatPolicy,
        core::LoggerInternalConsoleWritePolicy>
        Logger;

public:
    static const size_t MAX_HISTORY_ENTRIES = 64;
    static const size_t VAR_MAX = 1024;

    static const char* CMD_HISTORY_FILE_NAME;

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

    consoleState::Enum getVisState(void) const X_FINAL;

    // input callbacks
    bool onInputEvent(const input::InputEvent& event);

    ICVar* registerString(const char* pName, const char* Value, VarFlags flags, const char* pDesc) X_FINAL;
    ICVar* registerInt(const char* pName, int Value, int Min, int Max, VarFlags flags, const char* pDesc) X_FINAL;
    ICVar* registerFloat(const char* pName, float Value, float Min, float Max, VarFlags flags, const char* pDesc) X_FINAL;

    // refrenced based, these are useful if we want to use the value alot so we just register it's address.
    ICVar* registerRef(const char* pName, float* src, float defaultvalue, float Min, float Max, VarFlags flags, const char* pDesc) X_FINAL;
    ICVar* registerRef(const char* pName, int* src, int defaultvalue, int Min, int Max, VarFlags flags, const char* pDesc) X_FINAL;
    ICVar* registerRef(const char* pName, Color* src, Color defaultvalue, VarFlags flags, const char* pDesc) X_FINAL;
    ICVar* registerRef(const char* pName, Vec3f* src, Vec3f defaultvalue, VarFlags flags, const char* pDesc) X_FINAL;

    ICVar* getCVar(const char* pName) X_FINAL;

    void unregisterVariable(const char* pVarName) X_FINAL;
    void unregisterVariable(ICVar* pVar) X_FINAL;

    void registerCommand(const char* pName, ConsoleCmdFunc func, VarFlags Flags, const char* pDesc) X_FINAL;
    void unRegisterCommand(const char* pName) X_FINAL;

    void exec(const char* pCommand) X_FINAL;

    bool loadAndExecConfigFile(const char* pFileName) X_FINAL;

    // ICoreEventListener
    void OnCoreEvent(const CoreEventData& ed) X_FINAL;
    // ~ICoreEventListener

    void addLineToLog(const char* pStr, uint32_t length) X_FINAL;
    int32_t getLineCount(void) const X_FINAL;

    X_INLINE void showConsole(consoleState::Enum state);
    X_INLINE bool isVisable(void) const;
    X_INLINE bool isExpanded(void) const;
    X_INLINE void toggleConsole(bool expand);

private:
    void addCmd(const char* pCommand, ExecSource::Enum src, bool silent);
    void addCmd(const string& command, ExecSource::Enum src, bool silent);
    void executeStringInternal(const ExecCommand& cmd); // executes a command string, may contain multiple commands
    // runs a single cmd. even tho it's 'const' a command may alter the consoles state. :| aka 'clearBinds', 'consoleHide', ...
    void ExecuteCommand(const ConsoleCommand& cmd, core::StackString<ConsoleCommandArgs::MAX_STRING_CHARS>& str) const;

    ICVar* getCVarForRegistration(const char* pName);

    void registerVar(ICVar* pCVar);

    void listCommands(const char* searchPatten = nullptr);
    void listVariables(const char* searchPatten = nullptr);
    void listVariablesValues(const char* searchPatten = nullptr);

    void displayVarValue(const ICVar* pVar);
    void displayVarInfo(const ICVar* pVar, bool full = false);

    void addInputChar(const char c);
    void removeInputChar(bool bBackSpace);
    void clearInputBuffer(void);

    bool handleInput(const input::InputEvent& event);
    bool handleInputChar(const input::InputEvent& event);

    bool processInput(const input::InputEvent& event);

    void executeInputBuffer(void);

    // History
    void saveCmdHistory(void) const;
    void loadCmdHistory(bool async);
    void historyIoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
        core::XFileAsync* pFile, uint32_t bytesTransferred);
    void parseCmdHistory(const char* pBegin, const char* pEnd);
    void addCmdToHistory(const char* pCommand);
    void addCmdToHistory(const string& command);
    void resetHistoryPos(void);
    const char* getHistory(CmdHistory::Enum direction);

    // Binds a cmd to a key
    void addBind(const char* pKey, const char* pCmd);

    // returns the command for a given key
    // returns null if no bind found
    const char* findBind(const char* pKey);

    // removes all the binds.
    void clearAllBinds(void);

    void listbinds(IKeyBindDumpSink* CallBack);

    void configExec(const char* pCommand, const char* pEnd);

    // scrool helpers
private:
    void pageUp(void);
    void pageDown(void);
    void validateScrollPos(void);

private:
    // AutoComplete
    void resetAutoCompletion(void);
    void drawInputTxt(const Vec2f& start);

    int32_t autoCompleteNum_;
    int32_t autoCompleteIdx_;
    bool autoCompleteSelect_;

    X_INLINE bool isAutocompleteVis(void);

    // returns the max log lines that fit on screen.
    int32_t maxVisibleLogLines(void) const;

private:
    void drawBuffer(void);
    void drawScrollBar(void);

    void copy(void);
    void paste(void);

private:
    static bool cvarModifyBegin(ICVar* pCVar, ExecSource::Enum source);

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

    ExecCmdList cmds_;

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

#if X_ENABLE_CONFIG_HOT_RELOAD
    bool ignoreHotReload_;
#endif // !X_ENABLE_CONFIG_HOT_RELOAD

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
#pragma once

#ifndef _X_CONSOLE_DEF_H_
#define _X_CONSOLE_DEF_H_

#include <IConsole.h>
#include <IInput.h>
#include <ITexture.h>
#include <IDirectoryWatcher.h>

X_DISABLE_WARNING(4702)
#include <map>

// #include <string>
// #include <vector>
#include <list>
#include <deque>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
X_ENABLE_WARNING(4702)


#include <String\StrRef.h>

#include <Time\TimeVal.h>

#include "Logging\Logger.h"
#include "Logging\FilterPolicies\LoggerNoFilterPolicy.h"
#include "Logging\FormatPolicies\LoggerInternalConsoleFormatPolicy.h"
#include "Logging\WritePolicies\LoggerInternalConsoleWritePolicy.h"

#include <Containers\HashMap.h>

#include <Memory\AllocationPolicies\PoolAllocator.h>


X_NAMESPACE_BEGIN(core)

// do i want config files to hot reload.
// porbs only for testing, might be anoying otherwise.
#if X_DEBUG
#define X_ENABLE_CONFIG_HOT_RELOAD 1
#else
#define X_ENABLE_CONFIG_HOT_RELOAD 0
#endif // !X_DEBUG

typedef core::MemoryArena<
	core::PoolAllocator, 
	core::SingleThreadPolicy,
	core::SimpleBoundsChecking,
	core::SimpleMemoryTracking, 
	core::SimpleMemoryTagging> VarPool;

struct equal_to_case_insen
{
	bool operator()(const char* const _Left, const char* const _Right) const {
		return core::strUtil::IsEqualCaseInsen(_Left, _Right);
	}
};

struct ConsoleCommand
{
	ConsoleCommand(); 

	typedef Flags<VarFlag> FlagType;

	string Name;	// the name of the command.
	string Desc;	// descption text for the command

	FlagType			Flags; // flags like CHEAT etc..
	ConsoleCmdFunc		func; // Pointer to console command.
};

struct ConsoleCommandArgs : public IConsoleCmdArgs
{
	static const size_t	MAX_STRING_CHARS = 1024;
	static const size_t	MAX_COMMAND_ARGS = 64;
	static const size_t	MAX_COMMAND_STRING = 2 * MAX_STRING_CHARS;

public:
	explicit ConsoleCommandArgs(core::StackString<ConsoleCommandArgs::MAX_STRING_CHARS>& line);
	~ConsoleCommandArgs() X_OVERRIDE;

	virtual size_t GetArgCount(void) const X_OVERRIDE;
	virtual const char* GetArg(size_t idx) const X_OVERRIDE;
	void TokenizeString(const char* begin, const char* end);

private:
	size_t		argNum_;								// number of arguments
	char *	argv_[MAX_COMMAND_ARGS];				// points into tokenized
	char	tokenized_[MAX_COMMAND_STRING];		// will have terminator bytes inserted
};



X_DECLARE_ENUM(CmdHistory) (
	UP,
	DOWN
);

X_DECLARE_FLAGS(ExecSource)(
	CONSOLE,
	CONFIG,
	SYSTEM
);


class XConsole : 
	public IConsole, 
	public input::IInputEventListner,
	public core::IXHotReload
{
public:
	static const size_t MAX_HISTORY_ENTRIES = 64;
	static const size_t CONSOLE_LOG_LINE_HIEGHT = 20;

	static const size_t VAR_MAX = 4096;

	static const char* CMD_HISTORY_FILE_NAME;
	static const char* CONFIG_FILE_EXTENSION;

	typedef consoleState consoleState;
public:
	XConsole();

	virtual ~XConsole();

	virtual void Startup(ICore* pCore, bool basic) X_FINAL;
	virtual void RegisterCommnads(void) X_FINAL;
	virtual void ShutDown(void) X_FINAL;
	virtual void SaveChangedVars(void) X_FINAL;
	virtual void unregisterInputListener(void) X_FINAL;
	virtual void freeRenderResources(void) X_FINAL;

	virtual void dispatchRepeateInputEvents(void) X_FINAL;
	virtual void runDeferredCmds(void) X_FINAL;
	virtual void draw(void) X_FINAL;

	virtual consoleState::Enum getVisState(void) const X_FINAL;


	// input callbacks
	virtual bool OnInputEvent(const input::InputEvent& event) X_FINAL;
	virtual bool OnInputEventChar(const input::InputEvent& event) X_FINAL;


	virtual ICVar* RegisterString(const char* Name, const char* Value, int Flags, const char* desc) X_FINAL;
	virtual ICVar* RegisterInt(const char* Name, int Value, int Min, int Max, int Flags, const char* desc) X_FINAL;
	virtual ICVar* RegisterFloat(const char* Name, float Value, float Min, float Max, int Flags, const char* desc) X_FINAL;

	virtual ICVar* ConfigRegisterString(const char* Name, const char* Value, int Flags, const char* desc) X_FINAL;
	virtual ICVar* ConfigRegisterInt(const char* Name, int Value, int Min, int Max, int Flags, const char* desc) X_FINAL;
	virtual ICVar* ConfigRegisterFloat(const char* Name, float Value, float Min, float Max, int Flags, const char* desc) X_FINAL;


	// refrenced based, these are useful if we want to use the value alot so we just register it's address.
	virtual ICVar* Register(const char* name, float* src, float defaultvalue, float Min, float Max, int nFlags, const char* desc) X_FINAL;
	virtual ICVar* Register(const char* name, int* src, int defaultvalue, int Min, int Max, int nFlags, const char* desc) X_FINAL;
	virtual ICVar* Register(const char* name, Color* src, Color defaultvalue, int nFlags, const char* desc) X_FINAL;
	virtual ICVar* Register(const char* name, Vec3f* src, Vec3f defaultvalue, int flags, const char* desc) X_FINAL;


	virtual ICVar* GetCVar(const char* name) X_FINAL;

	virtual void UnregisterVariable(const char* sVarName) X_FINAL;

	virtual void AddCommand(const char* Name, ConsoleCmdFunc func, int Flags, const char* desc) X_FINAL;

	virtual void RemoveCommand(const char* Name) X_FINAL;

	virtual void Exec(const char* command, const bool DeferExecution) X_FINAL;

//	virtual void ConfigExec(const char* command) X_FINAL;
	virtual bool LoadConfig(const char* fileName) X_FINAL;

	// IXHotReload
	virtual void OnFileChange(const core::Path<char>& name) X_FINAL;
	// ~IXHotReload

	virtual void addLineToLog(const char* pStr, uint32_t length) X_FINAL;
	virtual int getLineCount(void) const X_FINAL;


	X_INLINE void ShowConsole(consoleState::Enum state);
	X_INLINE bool isVisable(void) const;
	X_INLINE bool isExpanded(void) const;
	X_INLINE void ToggleConsole(bool expand = false);

private:
	void LoadRenderResources(void);
	void RegisterInputListener(void);

	void ExecuteStringInternal(const char* command, ExecSource::Enum source = ExecSource::CONSOLE, const bool bSilentMode = false);
	void ExecuteCommand(ConsoleCommand &cmd, core::StackString<ConsoleCommandArgs::MAX_STRING_CHARS>& str);

	ICVar* GetCVarForRegistration(const char* Name);

	void RegisterVar(ICVar* pCVar);

	void ListCommands(const char* searchPatten = nullptr);
	void ListVariables(const char* searchPatten = nullptr);

	void DisplayVarValue(ICVar* pVar);
	void DisplayVarInfo(ICVar* pVar);

	void ExecuteDeferredCommands();

	void AddInputChar(const char c);
	void RemoveInputChar(bool bBackSpace);
	void ClearInputBuffer(void);

	bool ProcessInput(const input::InputEvent& event);

	void ExecuteInputBuffer(void);

	void SaveCmdHistory(void) const;
	void LoadCmdHistory(void);
	void AddCmdToHistory(const char* Command);
	void ResetHistoryPos(void);
	const char* GetHistory(CmdHistory::Enum direction);

	// Binds a cmd to a key
	void AddBind(const char* key, const char* cmd);

	// returns the command for a given key
	// returns null if no bind found
	const char* FindBind(const char* key);

	// removes all the binds.
	void ClearAllBinds(void);

	void Listbinds(IKeyBindDumpSink* CallBack);

	void ConfigExec(const char* command);

	// scrool helpers
private:
	void PageUp(void);
	void PageDown(void);
	void ValidateScrollPos(void);

private:
	// AutoComplete
	void ResetAutoCompletion(void);
	void DrawInputTxt(const Vec2f& start);

	int autoCompleteNum_;
	int autoCompleteIdx_;
	bool autoCompleteSelect_;

	X_INLINE bool isAutocompleteVis(void);

	// returns the max log lines that fit on screen.
	size_t MaxVisibleLogLines(void) const;

private:
	void DrawBuffer(void);
	void DrawScrollBar(void);

	void Copy(void);
	void Paste(void);

private:
	static bool CvarModifyBegin(ICVar* pCVar, ExecSource::Enum source);
			
private:

	struct DeferredCommand
	{
		DeferredCommand(const string& command, bool silentMode);

		string		command;
		bool		silentMode;
	};

	struct Cursor
	{
		Cursor();

		float curTime;
		float displayTime;
		bool draw;
	};


	// members.
	typedef core::HashMap<const char*, ICVar*, core::hash<const char*>, equal_to_case_insen> ConsoleVarMap;		// key points into string stored in ICVar or in .exe/.dll
	typedef ConsoleVarMap::iterator ConsoleVarMapItor;

	typedef core::HashMap<string, ConsoleCommand, core::hash<const char*>, equal_to_case_insen> ConsoleCmdMap;
	typedef ConsoleCmdMap::iterator ConsoleCmdMapItor;

	typedef core::HashMap<string, string, core::hash<const char*>, equal_to_case_insen> ConfigCmdsMap;

	typedef core::HashMap<string, string> ConsoleBindMap;
	typedef ConsoleBindMap::iterator ConsoleBindMapItor;

	typedef std::list<DeferredCommand> DeferredCmdList;

	typedef std::deque<string> ConsoleBuffer;
	typedef ConsoleBuffer::iterator ConsoleBufferItor;
	typedef ConsoleBuffer::reverse_iterator ConsoleBufferRItor;

	typedef core::Logger<
		core::LoggerNoFilterPolicy,
		core::LoggerInternalConsoleFormatPolicy,
		core::LoggerInternalConsoleWritePolicy> Logger;

	core::HeapArea			varHeap_;
	core::PoolAllocator		varAllocator_;
	VarPool					varArena_;

	Logger					logger_;

	ConsoleBuffer			CmdHistory_;
	// some sort of lock free ring buffer might work better for this log.
	ConsoleBuffer			ConsoleLog_; 
	core::Spinlock			logLock_;

	ConsoleVarMap			VarMap_;
	ConsoleCmdMap			CmdMap_;
	ConsoleBindMap			Binds_; // support sexy bind.
	ConfigCmdsMap			configCmds_;

	DeferredCmdList			deferredCmds_;

	TimeVal					WaitSeconds_;

	consoleState::Enum		consoleState_;

	int32_t					HistoryPos_;
	int32_t					CursorPos_;
	int32_t					ScrollPos_;
	string					InputBuffer_;
	string                  RefString_;

	X_DISABLE_WARNING(4324);
	input::InputEvent		repeatEvent_;
	X_ENABLE_WARNING(4324);

	float					repeatEventInterval_;
	float					repeatEventInitialDelay_;
	float					repeatEventTimer_;

	ICore*					pCore_;
	font::IFFont*			pFont_;
	render::IRender*		pRender_;
	input::IInput*			pInput_;

	texture::ITexture*		pBackground_;

	Cursor					cursor_;

#if X_ENABLE_CONFIG_HOT_RELOAD
	bool					ignoreHotReload_;
#endif // !X_ENABLE_CONFIG_HOT_RELOAD

private:
	static int		console_debug;
	static int		console_case_sensitive;
	static int		console_save_history;
	static Color	console_input_box_color;
	static Color	console_input_box_color_border;
	static Color	console_output_box_color;
	static Color	console_output_box_color_border;
	static Color	console_output_box_channel_color;
	static Color	console_output_scroll_bar_color;
	static Color	console_output_scroll_bar_slider_color;
	static int		console_output_draw_channel;
	static int		console_buffer_size;
	static int		console_cursor_skip_color_codes;

	// some behaviour options.
	static int		console_disable_mouse;

private:
	void Command_Exit(IConsoleCmdArgs* Cmd);
	void Command_Exec(IConsoleCmdArgs* Cmd);
	void Command_Help(IConsoleCmdArgs* Cmd);
	void Command_ListCmd(IConsoleCmdArgs* Cmd);
	void Command_ListDvars(IConsoleCmdArgs* Cmd);
	void Command_Echo(IConsoleCmdArgs* Cmd);
	void Command_Wait(IConsoleCmdArgs* Cmd);
	void Command_VarReset(IConsoleCmdArgs* Cmd);
	void Command_Bind(IConsoleCmdArgs* Cmd);
	void Command_BindsClear(IConsoleCmdArgs* Cmd);
	void Command_BindsList(IConsoleCmdArgs* Cmd);
	void Command_SetVarArchive(IConsoleCmdArgs* Cmd);
	void Command_ConsoleShow(IConsoleCmdArgs* Cmd);
	void Command_ConsoleHide(IConsoleCmdArgs* Cmd);
	void Command_ConsoleToggle(IConsoleCmdArgs* Cmd);
	void Command_SaveModifiedVars(IConsoleCmdArgs* Cmd);

};

X_NAMESPACE_END

#include "Console.inl"

#endif // _X_CONSOLE_DEF_H_
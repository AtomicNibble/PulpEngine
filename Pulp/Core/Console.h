#pragma once

#ifndef _X_CONSOLE_DEF_H_
#define _X_CONSOLE_DEF_H_

#include <IConsole.h>
#include <IInput.h>
#include <ITexture.h>

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

namespace stl
{
	//////////////////////////////////////////////////////////////////////////
	//! Searches the given entry in the map by key, and if there is none, returns the default value
	//////////////////////////////////////////////////////////////////////////
	template <typename Map>
	inline typename Map::mapped_type find_in_map(const Map& mapKeyToValue, const typename Map::key_type& key, typename Map::mapped_type valueDefault)
	{
		typename Map::const_iterator it = mapKeyToValue.find(key);
		if (it == mapKeyToValue.end())
			return valueDefault;
		else
			return it->second;
	}
}


struct ConsoleCommand
{
	ConsoleCommand() : pFunc(0) {} // flags default con is (0)

	typedef Flags<VarFlag> FlagType;

	string Name;	// the name of the command.
	string Desc;	// descption text for the command

	FlagType			Flags; // flags like CHEAT etc..
	ConsoleCmdFunc		pFunc; // Pointer to console command.
};

struct ConsoleCommandArgs : public IConsoleCmdArgs
{
	static const int	MAX_STRING_CHARS = 1024;
	static const int	MAX_COMMAND_ARGS = 64;
	static const int	MAX_COMMAND_STRING = 2 * MAX_STRING_CHARS;

public:
	ConsoleCommandArgs(core::StackString<ConsoleCommandArgs::MAX_STRING_CHARS>& line) {
		TokenizeString(line.begin(), line.end()); 
	}
	~ConsoleCommandArgs() X_OVERRIDE{}

	virtual int GetArgCount(void) const X_OVERRIDE {
		return argNum_;
	}
		virtual const char* GetArg(int Idx) const X_OVERRIDE {
		return (argNum_ >= 0 && Idx < argNum_) ? argv_[Idx] : "";
	}

	void TokenizeString(const char* begin, const char* end);

private:
	int		argNum_;								// number of arguments
	char *	argv_[MAX_COMMAND_ARGS];				// points into tokenized
	char	tokenized_[MAX_COMMAND_STRING];		// will have terminator bytes inserted
};


struct CmdHistory
{
	enum Enum
	{
		UP,
		DOWN
	};
};

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

	struct consoleState
	{
		enum Enum
		{
			CLOSED,
			OPEN,
			EXPANDED
		};
	};
public:
	XConsole();

	virtual ~XConsole();

	virtual void Startup(ICore* pCore) X_FINAL;
	virtual void ShutDown(void) X_FINAL;
	virtual void unregisterInputListener(void) X_FINAL;
	virtual void freeRenderResources(void) X_FINAL;

	virtual void Draw(void) X_FINAL;

	// input callbacks
	virtual bool OnInputEvent(const input::InputEvent& event) X_FINAL;
	virtual bool OnInputEventChar(const input::InputEvent& event) X_FINAL;


	virtual ICVar* RegisterString(const char* Name, const char* Value, int Flags, const char* desc, ConsoleVarFunc pChangeFunc = 0) X_FINAL;
	virtual ICVar* RegisterInt(const char* Name, int Value, int Min, int Max, int Flags, const char* desc, ConsoleVarFunc pChangeFunc = 0) X_FINAL;
	virtual ICVar* RegisterFloat(const char* Name, float Value, float Min, float Max, int Flags, const char* desc, ConsoleVarFunc pChangeFunc = 0) X_FINAL;

	virtual ICVar* ConfigRegisterString(const char* Name, const char* Value, int Flags, const char* desc) X_FINAL;
	virtual ICVar* ConfigRegisterInt(const char* Name, int Value, int Min, int Max, int Flags, const char* desc) X_FINAL;
	virtual ICVar* ConfigRegisterFloat(const char* Name, float Value, float Min, float Max, int Flags, const char* desc) X_FINAL;


	// refrenced based, these are useful if we want to use the value alot so we just register it's address.
	virtual ICVar* Register(const char* name, float* src, float defaultvalue, float Min, float Max, int nFlags, const char* desc, ConsoleVarFunc pChangeFunc = 0) X_FINAL;
	virtual ICVar* Register(const char* name, int* src, int defaultvalue, int Min, int Max, int nFlags, const char* desc, ConsoleVarFunc pChangeFunc = 0) X_FINAL;
	virtual ICVar* Register(const char* name, Color* src, Color defaultvalue, int nFlags, const char* desc, ConsoleVarFunc pChangeFunc = 0) X_FINAL;
	virtual ICVar* Register(const char* name, Vec3f* src, Vec3f defaultvalue, int flags, const char* desc, ConsoleVarFunc pChangeFunc = 0) X_FINAL;


	virtual ICVar* GetCVar(const char* name) X_FINAL;

	virtual void UnregisterVariable(const char* sVarName) X_FINAL;

	virtual void AddCommand(const char* Name, ConsoleCmdFunc func, int Flags, const char* desc) X_FINAL;

	virtual void RemoveCommand(const char* Name) X_FINAL;

	virtual void Exec(const char* command, const bool DeferExecution) X_FINAL;

//	virtual void ConfigExec(const char* command) X_FINAL;
	virtual bool LoadConfig(const char* fileName) X_FINAL;

	// IXHotReload
	virtual bool OnFileChange(const char* name) X_FINAL;
	// ~IXHotReload

	void OnFrameBegin() X_FINAL;

	X_INLINE void ShowConsole(consoleState::Enum state) {
		consoleState_ = state;
	}
	X_INLINE bool isVisable(void) const {
		return consoleState_ != consoleState::CLOSED;
	}
	X_INLINE bool isExpanded(void) const {
		return consoleState_ == consoleState::EXPANDED;
	}
	X_INLINE void ToggleConsole(bool expand = false) {
		if (isVisable())
			consoleState_ = consoleState::CLOSED;
		else {
			if (expand)
				consoleState_ = consoleState::EXPANDED;
			else
				consoleState_ = consoleState::OPEN;
		}
	}

protected:
	void ExecuteStringInternal(const char *command, ExecSource::Enum source = ExecSource::CONSOLE, const bool bSilentMode = false);
	void ExecuteCommand(ConsoleCommand &cmd, core::StackString<ConsoleCommandArgs::MAX_STRING_CHARS>& str);

	ICVar* GetCVarForRegistration(const char* Name);

	void RegisterVar(ICVar *pCVar, ConsoleVarFunc pChangeFunc);

	void ListCommands(const char* searchPatten = nullptr);
	void ListVariables(const char* searchPatten = nullptr);

	void DisplayVarValue(ICVar *pVar);
	void DisplayVarInfo(ICVar *pVar);

	void ExecuteDeferredCommands();

	void AddInputChar(const char c);
	void RemoveInputChar(bool bBackSpace);
	void ClearInputBuffer(void);

	bool ProcessInput(const input::InputEvent& event);

	void ExecuteInputBuffer();

	void AddCmdToHistory(const char* Command);

	const char* GetHistory(CmdHistory::Enum direction);

	// Binds a cmd to a key
	void AddBind(const char* key, const char* cmd);

	// returns the command for a given key
	// returns null if no bind found
	const char* FindBind(const char* key);

	// removes all the binds.
	void ClearAllBinds();

	void Listbinds(IKeyBindDumpSink* CallBack);

	void ConfigExec(const char* command);

public:

	virtual void addLineToLog(const char* pStr, uint32_t length) X_FINAL;
	virtual int getLineCount(void) const X_FINAL;


private:
	// AutoComplete
	void ResetAutoCompletion(void);
	void DrawInputTxt(const Vec2f& start);

	int autoCompleteNum_;
	int autoCompleteIdx_;
	bool autoCompleteSelect_;

	X_INLINE bool isAutocompleteVis(void) {
		return autoCompleteNum_ > 0;
	}

private:
	void DrawBuffer();
	void DrawScrollBar();

	void Copy(void);
	void Paste(void);

private:
	static bool CvarModifyBegin(ICVar* pCVar, ExecSource::Enum source);
			
private:

	struct DeferredCommand
	{
		string		command;
		bool		silentMode;

		DeferredCommand(const string& command, bool silentMode)
			: command(command), silentMode(silentMode)
		{}
	};

	// members.
	typedef core::HashMap<const char*, ICVar*, core::hash<const char*>, equal_to_case_insen> ConsoleVarMap;		// key points into string stored in ICVar or in .exe/.dll
	typedef ConsoleVarMap::iterator ConsoleVarMapItor;

	typedef core::HashMap<string, ConsoleCommand, core::hash<const char*>, equal_to_case_insen> ConsoleCmdMap;
	typedef ConsoleCmdMap::iterator ConsoleCmdMapItor;

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
	ConsoleBuffer			ConsoleLog_;

	ConsoleVarMap			VarMap_;
	ConsoleCmdMap			CmdMap_;
	ConsoleBindMap			Binds_; // support sexy bind.

	DeferredCmdList			deferredCmds_;

	TimeVal					WaitSeconds_;

	consoleState::Enum		consoleState_;

	int32_t					HistoryPos_;
	int32_t					CursorPos_;
	int32_t					ScrollPos_;
	string					InputBuffer_;
	string                  RefString_;

	input::InputEvent		repeatEvent_;
	float					repeatEventInterval_;
	float					repeatEventInitialDelay_;
	float					repeatEventTimer_;

	ICore*					pCore_;
	font::IFFont*			pFont_;
	render::IRender*		pRender_;
	input::IInput*			pInput_;

	texture::ITexture*		pBackground_;

	struct Cursor_t
	{
		Cursor_t() : curTime(0.f), displayTime(0.5f), draw(false) {}
		float curTime;
		float displayTime;
		bool draw;
	}cursor_;

private:
	static int		console_debug;
	static Color	console_input_box_color;
	static Color	console_input_box_color_border;
	static Color	console_output_box_color;
	static Color	console_output_box_color_border;
	static Color	console_output_box_channel_color;
	static int		console_output_draw_channel;
	static int		console_buffer_size;

	friend void Command_Exit(IConsoleCmdArgs* Cmd);
	friend void Command_Exec(IConsoleCmdArgs* Cmd);
	friend void Command_Help(IConsoleCmdArgs* Cmd);
	friend void Command_ListCmd(IConsoleCmdArgs* Cmd);
	friend void Command_ListDvars(IConsoleCmdArgs* Cmd);
	friend void Command_Echo(IConsoleCmdArgs* Cmd);
	friend void Command_Wait(IConsoleCmdArgs* Cmd);
	friend void Command_VarReset(IConsoleCmdArgs* Cmd);
	friend void Command_Bind(IConsoleCmdArgs* Cmd);
	friend void Command_BindsClear(IConsoleCmdArgs* Cmd);
	friend void Command_BindsList(IConsoleCmdArgs* Cmd);
	friend void Command_SetVarArchive(IConsoleCmdArgs* Cmd);
	friend void Command_ConsoleShow(IConsoleCmdArgs* Cmd);
	friend void Command_ConsoleHide(IConsoleCmdArgs* Cmd);
	friend void Command_ConsoleToggle(IConsoleCmdArgs* Cmd);

};

X_NAMESPACE_END

#endif // _X_CONSOLE_DEF_H_
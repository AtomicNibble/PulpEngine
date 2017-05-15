#pragma once


#ifndef _X_CONSOLE_INTER_H_
#define _X_CONSOLE_INTER_H_


#include <Prepro\PreproStringize.h>
#include <Util\Delegate.h>

#include <Traits\FunctionTraits.h>

struct ICore;

X_NAMESPACE_DECLARE(core,
	struct FrameTimeData;
);

X_NAMESPACE_BEGIN(core)


X_DECLARE_FLAGS(VarFlag) (
// ALL,			// all flags
INT,			// variable is an integer
FLOAT,			// variable is a float
STRING,			// variable is a string
COLOR,
VECTOR,
BITFIELD,

STATIC_DECL,	// statically declared, not user created
CHEAT,			// variable is considered a cheat
READONLY,		// display only, cannot be set by user / config
HIDDEN,			// not visable to the user.
ARCHIVE,		// set to cause it to be saved to a config file
MODIFIED,		// set when the variable is modified
CONFIG,			// loaded or set from a config file.

SAVE_IF_CHANGED, // saved to config if changed.
RESTART_REQUIRED,

SYSTEM,			// system related
TOOL,			// tool related
CPY_NAME		// makes a copy of the name
);

typedef Flags<VarFlag> VarFlags;

X_DECLARE_FLAG_OPERATORS(VarFlags);

X_DECLARE_ENUM(consoleState)(
	CLOSED,
	OPEN,
	EXPANDED
);


struct ICVar;

// console commands.
struct IConsoleCmdArgs
{
	virtual ~IConsoleCmdArgs() {}
	// Gets number of arguments supplied to the command (including the command itself)
	virtual size_t GetArgCount(void) const X_ABSTRACT;
	// Gets argument by index, idx must be in 0 <= idx < GetArgCount()
	virtual const char* GetArg(size_t idx) const X_ABSTRACT;
};

struct IKeyBindDumpSink
{
	virtual ~IKeyBindDumpSink(){}
	virtual void OnKeyBindFound(const char* Bind, const char* Command) X_ABSTRACT;
};


// typedef core::traits::Function<void(ICVar*)> ConsoleVarFunc;
typedef core::traits::Function<void(IConsoleCmdArgs*)> ConsoleCmdOldFunc;

typedef core::Delegate<void(ICVar*)> ConsoleVarFunc;
typedef core::Delegate<void(IConsoleCmdArgs*)> ConsoleCmdFunc;

// The console interface
struct IConsole
{
	virtual ~IConsole(){}


	virtual void registerVars(void) X_ABSTRACT;
	virtual void registerCmds(void) X_ABSTRACT;

	// called at start when not much else exists, just so subsystems can register vars
	virtual bool init(ICore* pCore, bool basic) X_ABSTRACT;
	// finialize any async init tasks.
	virtual bool asyncInitFinalize(void) X_ABSTRACT;
	virtual bool registerInputListener(void) X_ABSTRACT;
	virtual bool loadRenderResources(void) X_ABSTRACT;


	virtual void shutDown(void) X_ABSTRACT;
	virtual void unregisterInputListener(void) X_ABSTRACT;
	virtual void freeRenderResources(void) X_ABSTRACT;
	virtual void saveChangedVars(void) X_ABSTRACT; // saves vars with 'SAVE_IF_CHANGED' if modified.

	// console set's it's own input repeat rate, that's timed instead of every frame.
	virtual void Job_dispatchRepeateInputEvents(core::FrameTimeData& time) X_ABSTRACT;
	virtual void Job_runCmds(void) X_ABSTRACT;
	virtual void draw(core::FrameTimeData& time) X_ABSTRACT;

	virtual consoleState::Enum getVisState(void) const X_ABSTRACT;

	// Register variables.
	virtual ICVar* RegisterString(const char* pName, const char* Value, VarFlags flags, const char* desc) X_ABSTRACT;
	virtual ICVar* RegisterInt(const char* pName, int Value, int Min, int Max, VarFlags flags, const char* desc) X_ABSTRACT;
	virtual ICVar* RegisterFloat(const char* pName, float Value, float Min, float Max, VarFlags flags, const char* desc) X_ABSTRACT;

	// refrenced based, these are useful if we want to use the value alot so we just register it's address.
	virtual ICVar* Register(const char* pName, float* src, float defaultvalue, float Min, float Max, VarFlags flags, const char* desc) X_ABSTRACT;
	virtual ICVar* Register(const char* pName, int* src, int defaultvalue, int Min, int Max, VarFlags flags, const char* desc) X_ABSTRACT;
	virtual ICVar* Register(const char* pName, Color* src, Color defaultvalue, VarFlags flags, const char* desc) X_ABSTRACT;
	virtual ICVar* Register(const char* pName, Vec3f* src, Vec3f defaultvalue, VarFlags flags, const char* desc) X_ABSTRACT;


	virtual ICVar* GetCVar(const char* pName) X_ABSTRACT;

	virtual void UnregisterVariable(const char* pVarName) X_ABSTRACT;

	virtual void RegisterCommand(const char* pName, ConsoleCmdFunc func, VarFlags Flags, const char* desc) X_ABSTRACT;
	virtual void UnRegisterCommand(const char* pName) X_ABSTRACT;

	virtual void Exec(const char* pCommand) X_ABSTRACT;

//	virtual void ConfigExec(const char* command) X_ABSTRACT;
	virtual bool LoadAndExecConfigFile(const char* pFileName) X_ABSTRACT;

	// Logging
	virtual void addLineToLog(const char* pStr, uint32_t length) X_ABSTRACT;
	virtual int getLineCount(void) const X_ABSTRACT;
	// ~Logging
};



struct ICVar
{
	typedef VarFlags FlagType;
	typedef char StrBuf[128];

	virtual ~ICVar() {}

	virtual const char* GetName(void) const X_ABSTRACT;
	virtual const char* GetDesc(void) const X_ABSTRACT;
	virtual const char* GetDefaultStr(StrBuf& buf) const X_ABSTRACT;
	virtual void SetDesc(const char* pDesc) X_ABSTRACT;

	virtual int GetInteger(void) const X_ABSTRACT;
	virtual float GetFloat(void) const X_ABSTRACT;
	virtual const char* GetString(StrBuf& buf) const X_ABSTRACT;

	virtual void SetDefault(const char* s) X_ABSTRACT;
	virtual void Set(const char* s) X_ABSTRACT;
	virtual void ForceSet(const char* s) X_ABSTRACT;

	virtual void Set(const float f) X_ABSTRACT;
	virtual void Set(const int i) X_ABSTRACT;

	virtual FlagType GetFlags(void) const X_ABSTRACT;
	virtual FlagType SetFlags(FlagType flags) X_ABSTRACT;
	virtual void SetModified(void) X_ABSTRACT;
	virtual float GetMin(void) const X_ABSTRACT;
	virtual float GetMax(void) const X_ABSTRACT;
	virtual int32_t GetMinInt(void) const X_ABSTRACT;
	virtual int32_t GetMaxInt(void) const X_ABSTRACT;
	virtual int32_t GetDefaultInt(void) const X_ABSTRACT;

	virtual VarFlag::Enum GetType(void) const X_ABSTRACT;

	virtual void Release(void) X_ABSTRACT;
	virtual void Reset(void) X_ABSTRACT; // reset to default value.

	virtual ICVar* SetOnChangeCallback(ConsoleVarFunc pChangeFunc) X_ABSTRACT;
	virtual ConsoleVarFunc GetOnChangeCallback(void) const X_ABSTRACT;
};


X_NAMESPACE_END

#endif // !_X_CONSOLE_INTER_H_
#pragma once


#ifndef _X_CONSOLE_INTER_H_
#define _X_CONSOLE_INTER_H_


#include <Prepro\PreproStringize.h>

struct ICore;

X_NAMESPACE_BEGIN(core)


X_DECLARE_FLAGS(VarFlag) (
// ALL,			// all flags
INT,			// variable is an integer
FLOAT,			// variable is a float
STRING,			// variable is a string
COLOR,
VECTOR,
BITFIELD,

STATIC,			// statically declared, not user created
CHEAT,			// variable is considered a cheat
READONLY,		// display only, cannot be set by user / config
HIDDEN,			// not visable to the user.
ARCHIVE,		// set to cause it to be saved to a config file
MODIFIED,		// set when the variable is modified
CONFIG,			// loaded or set from a config file.

SAVE_IF_CHANGED, // saved to config if changed.

SYSTEM,			// system related
TOOL,			// tool related
CPY_NAME		// makes a copy of the name
);


struct ICVar;

// console commands.
struct IConsoleCmdArgs
{
	virtual ~IConsoleCmdArgs(){}
	// Gets number of arguments supplied to the command (including the command itself)
	virtual int GetArgCount() const X_ABSTRACT;
	// Gets argument by index, nIndex must be in 0 <= nIndex < GetArgCount()
	virtual const char* GetArg(int nIndex) const X_ABSTRACT;
};

struct IKeyBindDumpSink
{
	virtual ~IKeyBindDumpSink(){}
	virtual void OnKeyBindFound(const char* Bind, const char* Command) X_ABSTRACT;
};


typedef void(*ConsoleCmdFunc)(IConsoleCmdArgs*);
typedef void(*ConsoleVarFunc)(ICVar*);


// The console interface
struct IConsole
{
	virtual ~IConsole(){}

	virtual void Startup(ICore* pCore) X_ABSTRACT;
	virtual void ShutDown() X_ABSTRACT;
	virtual void unregisterInputListener(void) X_ABSTRACT;
	virtual void freeRenderResources() X_ABSTRACT;

	virtual void Draw() X_ABSTRACT;

	// Register variables.
	virtual ICVar* RegisterString(const char* Name, const char* Value, int flags, const char* desc, ConsoleVarFunc pChangeFunc = 0) X_ABSTRACT;
	virtual ICVar* RegisterInt(const char* Name, int Value, int Min, int Max, int flags, const char* desc, ConsoleVarFunc pChangeFunc = 0) X_ABSTRACT;
	virtual ICVar* RegisterFloat(const char* Name, float Value, float Min, float Max, int flags, const char* desc, ConsoleVarFunc pChangeFunc = 0) X_ABSTRACT;

	virtual ICVar* ConfigRegisterString(const char* Name, const char* Value, int flags, const char* desc) X_ABSTRACT;
	virtual ICVar* ConfigRegisterInt(const char* Name, int Value, int Min, int Max, int flags, const char* desc) X_ABSTRACT;
	virtual ICVar* ConfigRegisterFloat(const char* Name, float Value, float Min, float Max, int flags, const char* desc) X_ABSTRACT;

	// refrenced based, these are useful if we want to use the value alot so we just register it's address.
	virtual ICVar* Register(const char* name, float* src, float defaultvalue, float Min, float Max, int flags, const char* desc, ConsoleVarFunc pChangeFunc = 0) X_ABSTRACT;
	virtual ICVar* Register(const char* name, int* src, int defaultvalue, int Min, int Max, int flags, const char* desc, ConsoleVarFunc pChangeFunc = 0) X_ABSTRACT;
	virtual ICVar* Register(const char* name, Color* src, Color defaultvalue, int flags, const char* desc, ConsoleVarFunc pChangeFunc = 0) X_ABSTRACT;
	virtual ICVar* Register(const char* name, Vec3f* src, Vec3f defaultvalue, int flags, const char* desc, ConsoleVarFunc pChangeFunc = 0) X_ABSTRACT;


	virtual ICVar* GetCVar(const char* name) X_ABSTRACT;

	virtual void UnregisterVariable(const char* sVarName) X_ABSTRACT;

	virtual void AddCommand(const char* Name, ConsoleCmdFunc func, int Flags, const char* desc) X_ABSTRACT;

	virtual void RemoveCommand(const char* sName) X_ABSTRACT;

	virtual void Exec(const char* command, const bool DeferExecution = false) X_ABSTRACT;

//	virtual void ConfigExec(const char* command) X_ABSTRACT;
	virtual bool LoadConfig(const char* fileName) X_ABSTRACT;

	virtual void OnFrameBegin() X_ABSTRACT;

	// Logging
	virtual void addLineToLog(const char* pStr, uint32_t length) X_ABSTRACT;
	virtual int getLineCount(void) const X_ABSTRACT;
	// ~Logging
};



struct ICVar
{
	typedef Flags<VarFlag> FlagType;

	virtual ~ICVar() {}

	virtual const char* GetName() const X_ABSTRACT;
	virtual const char* GetDesc() const X_ABSTRACT;
	virtual const char* GetDefaultStr() const X_ABSTRACT;

	virtual int GetInteger(void) const X_ABSTRACT;
	virtual float GetFloat(void) const X_ABSTRACT;
	virtual const char *GetString(void) X_ABSTRACT;

	virtual void SetDefault(const char* s) X_ABSTRACT;
	virtual void Set(const char* s) X_ABSTRACT;
	virtual void ForceSet(const char* s) X_ABSTRACT;

	virtual void Set(const float f) X_ABSTRACT;
	virtual void Set(const int i) X_ABSTRACT;

	virtual FlagType GetFlags() const X_ABSTRACT;
	virtual FlagType SetFlags(FlagType flags) X_ABSTRACT;
	virtual float GetMin(void) X_ABSTRACT;
	virtual float GetMax(void) X_ABSTRACT;

	virtual VarFlag::Enum GetType() X_ABSTRACT;

	virtual void Release() X_ABSTRACT;
	virtual void Reset() X_ABSTRACT; // reset to default value.

	virtual void SetOnChangeCallback(ConsoleVarFunc pChangeFunc) X_ABSTRACT;
	virtual ConsoleVarFunc GetOnChangeCallback() X_ABSTRACT;
};


X_NAMESPACE_END

#endif // !_X_CONSOLE_INTER_H_
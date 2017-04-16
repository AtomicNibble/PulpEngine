#pragma once

#ifndef _X_CONSOLE_NULL_DEF_H_
#define _X_CONSOLE_NULL_DEF_H_

#include <IConsole.h>

X_NAMESPACE_BEGIN(core)

class XConsoleNULL : public IConsole
{
public:
	static const size_t MAX_HISTORY_ENTRIES = 50;

public:
	XConsoleNULL();

	virtual ~XConsoleNULL() X_FINAL;

	virtual void registerCommnads(void) X_FINAL;

	virtual void startup(ICore* pCore, bool basic) X_FINAL;
	virtual bool asyncInitFinalize(void) X_FINAL;
	virtual void shutDown(void) X_FINAL;
	virtual void saveChangedVars(void) X_FINAL;
	virtual void unregisterInputListener(void) X_FINAL;
	virtual void freeRenderResources(void) X_FINAL;

	virtual void Job_dispatchRepeateInputEvents(core::FrameTimeData& time) X_FINAL;
	virtual void Job_runCmds(void) X_FINAL;
	virtual void draw(core::FrameTimeData& time) X_FINAL;

	virtual consoleState::Enum getVisState(void) const X_FINAL;

	virtual ICVar* RegisterString(const char* Name, const char* Value, VarFlags Flags, const char* desc) X_FINAL;
	virtual ICVar* RegisterInt(const char* Name, int Value, int Min, int Max, VarFlags Flags, const char* desc) X_FINAL;
	virtual ICVar* RegisterFloat(const char* Name, float Value, float Min, float Max, VarFlags Flags, const char* desc) X_FINAL;

	virtual ICVar* ConfigRegisterString(const char* Name, const char* Value, VarFlags Flags, const char* desc) X_FINAL;
	virtual ICVar* ConfigRegisterInt(const char* Name, int Value, int Min, int Max, VarFlags Flags, const char* desc) X_FINAL;
	virtual ICVar* ConfigRegisterFloat(const char* Name, float Value, float Min, float Max, VarFlags Flags, const char* desc) X_FINAL;


	// refrenced based, these are useful if we want to use the value alot so we just register it's address.
	virtual ICVar* Register(const char* name, float* src, float defaultvalue, float Min, float Max, VarFlags nFlags, const char* desc) X_FINAL;
	virtual ICVar* Register(const char* name, int* src, int defaultvalue, int Min, int Max, VarFlags nFlags, const char* desc) X_FINAL;
	virtual ICVar* Register(const char* name, Color* src, Color defaultvalue, VarFlags nFlags, const char* desc) X_FINAL;
	virtual ICVar* Register(const char* name, Vec3f* src, Vec3f defaultvalue, VarFlags flags, const char* desc) X_FINAL;

	virtual ICVar* GetCVar(const char* name) X_FINAL;

	virtual void UnregisterVariable(const char* sVarName) X_FINAL;

	virtual void RegisterCommand(const char* Name, ConsoleCmdFunc func, VarFlags Flags, const char* desc) X_FINAL;
	virtual void UnRegisterCommand(const char* Name) X_FINAL;

	virtual void Exec(const char* command) X_FINAL;

//	virtual void ConfigExec(const char* command) X_FINAL;
	virtual bool LoadAndExecConfigFile(const char* fileName) X_FINAL;

	// Loggging
	virtual void addLineToLog(const char* pStr, uint32_t length) X_FINAL;
	virtual int getLineCount(void) const X_FINAL;
	// ~Loggging

};

X_NAMESPACE_END

#endif // !_X_CONSOLE_NULL_DEF_H_
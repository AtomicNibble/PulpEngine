#pragma once

#ifndef _X_CONSOLE_NULL_DEF_H_
#define _X_CONSOLE_NULL_DEF_H_

#include <IConsole.h>

X_NAMESPACE_BEGIN(core)

class XConsoleNULL : public IConsole
{
public:
    XConsoleNULL();

    virtual ~XConsoleNULL() X_FINAL;

    virtual void registerVars(void) X_FINAL;
    virtual void registerCmds(void) X_FINAL;

    // called at start when not much else exists, just so subsystems can register vars
    virtual bool init(ICore* pCore, bool basic) X_FINAL;
    // finialize any async init tasks.
    virtual bool asyncInitFinalize(void) X_FINAL;
    virtual bool loadRenderResources(void) X_FINAL;

    virtual void shutDown(void) X_FINAL;
    virtual void freeRenderResources(void) X_FINAL;
    virtual void saveChangedVars(void) X_FINAL; // saves vars with 'SAVE_IF_CHANGED' if modified.

    virtual void dispatchRepeateInputEvents(core::FrameTimeData& time) X_FINAL;
    virtual void runCmds(void) X_FINAL;
    virtual void draw(core::FrameTimeData& time) X_FINAL;

    virtual consoleState::Enum getVisState(void) const X_FINAL;

    virtual ICVar* registerString(const char* Name, const char* Value, VarFlags Flags, const char* desc) X_FINAL;
    virtual ICVar* registerInt(const char* Name, int Value, int Min, int Max, VarFlags Flags, const char* desc) X_FINAL;
    virtual ICVar* registerFloat(const char* Name, float Value, float Min, float Max, VarFlags Flags, const char* desc) X_FINAL;

    // refrenced based, these are useful if we want to use the value alot so we just register it's address.
    virtual ICVar* registerRef(const char* name, float* src, float defaultvalue, float Min, float Max, VarFlags nFlags, const char* desc) X_FINAL;
    virtual ICVar* registerRef(const char* name, int* src, int defaultvalue, int Min, int Max, VarFlags nFlags, const char* desc) X_FINAL;
    virtual ICVar* registerRef(const char* name, Color* src, Color defaultvalue, VarFlags nFlags, const char* desc) X_FINAL;
    virtual ICVar* registerRef(const char* name, Vec3f* src, Vec3f defaultvalue, VarFlags flags, const char* desc) X_FINAL;

    virtual ICVar* getCVar(const char* name) X_FINAL;

    virtual void unregisterVariable(const char* sVarName) X_FINAL;
    virtual void unregisterVariable(ICVar* pVar) X_FINAL;

    virtual void registerCommand(const char* Name, ConsoleCmdFunc func, VarFlags Flags, const char* desc) X_FINAL;
    virtual void unRegisterCommand(const char* Name) X_FINAL;

    virtual void exec(const char* command) X_FINAL;

    //	virtual void ConfigExec(const char* command) X_FINAL;
    virtual bool loadAndExecConfigFile(const char* fileName) X_FINAL;

    // Loggging
    virtual void addLineToLog(const char* pStr, uint32_t length) X_FINAL;
    virtual int getLineCount(void) const X_FINAL;
    // ~Loggging
};

X_NAMESPACE_END

#endif // !_X_CONSOLE_NULL_DEF_H_
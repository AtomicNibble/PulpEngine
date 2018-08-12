#include "stdafx.h"
#include "ConsoleNULL.h"

X_NAMESPACE_BEGIN(core)

XConsoleNULL::XConsoleNULL()
{
}

XConsoleNULL::~XConsoleNULL()
{
}

void XConsoleNULL::registerVars(void)
{
}

void XConsoleNULL::registerCmds(void)
{
}

bool XConsoleNULL::init(ICore* pCore, bool basic)
{
    X_UNUSED(pCore);
    X_UNUSED(basic);
    return true;
}

bool XConsoleNULL::asyncInitFinalize(void)
{
    return true;
}

bool XConsoleNULL::loadRenderResources(void)
{
    return true;
}

void XConsoleNULL::shutDown(void)
{
}

void XConsoleNULL::freeRenderResources(void)
{
}

void XConsoleNULL::saveChangedVars(void)
{
}

void XConsoleNULL::Job_dispatchRepeateInputEvents(core::FrameTimeData& time)
{
    X_UNUSED(time);
}

void XConsoleNULL::Job_runCmds(void)
{
}

void XConsoleNULL::draw(core::FrameTimeData& time)
{
    X_UNUSED(time);
}

consoleState::Enum XConsoleNULL::getVisState(void) const
{
    return consoleState::CLOSED;
}

ICVar* XConsoleNULL::RegisterString(const char* Name, const char* Value, VarFlags Flags,
    const char* desc)
{
    X_UNUSED(Name);
    X_UNUSED(Value);
    X_UNUSED(Flags);
    X_UNUSED(desc);
    return nullptr;
}

ICVar* XConsoleNULL::RegisterInt(const char* Name, int Value, int Min, int Max,
    VarFlags Flags, const char* desc)
{
    X_UNUSED(Name);
    X_UNUSED(Value);
    X_UNUSED(Min);
    X_UNUSED(Max);
    X_UNUSED(Flags);
    X_UNUSED(desc);
    return nullptr;
}

ICVar* XConsoleNULL::RegisterFloat(const char* Name, float Value, float Min, float Max,
    VarFlags flags, const char* desc)
{
    X_UNUSED(Name);
    X_UNUSED(Value);
    X_UNUSED(Min);
    X_UNUSED(Max);
    X_UNUSED(flags);
    X_UNUSED(desc);
    return nullptr;
}

// refrenced based, these are useful if we want to use the value alot so we just register it's address.
ICVar* XConsoleNULL::Register(const char* name, float* src, float defaultvalue,
    float Min, float Max, VarFlags flags, const char* desc)
{
    X_UNUSED(name);
    X_UNUSED(src);
    X_UNUSED(defaultvalue);
    X_UNUSED(Min);
    X_UNUSED(Max);
    X_UNUSED(flags);
    X_UNUSED(desc);

    *src = defaultvalue;

    return nullptr;
}

ICVar* XConsoleNULL::Register(const char* name, int* src, int defaultvalue,
    int Min, int Max, VarFlags flags, const char* desc)
{
    X_UNUSED(name);
    X_UNUSED(src);
    X_UNUSED(defaultvalue);
    X_UNUSED(Min);
    X_UNUSED(Max);
    X_UNUSED(flags);
    X_UNUSED(desc);

    *src = defaultvalue;

    return nullptr;
}

ICVar* XConsoleNULL::Register(const char* name, Color* src, Color defaultvalue,
    VarFlags flags, const char* desc)
{
    X_UNUSED(name);
    X_UNUSED(src);
    X_UNUSED(defaultvalue);
    X_UNUSED(flags);
    X_UNUSED(desc);

    *src = defaultvalue;
    return nullptr;
}

ICVar* XConsoleNULL::Register(const char* name, Vec3f* src, Vec3f defaultvalue,
    VarFlags flags, const char* desc)
{
    X_UNUSED(name);
    X_UNUSED(src);
    X_UNUSED(defaultvalue);
    X_UNUSED(flags);
    X_UNUSED(desc);

    *src = defaultvalue;

    return nullptr;
}

ICVar* XConsoleNULL::GetCVar(const char* name)
{
    X_UNUSED(name);

    return nullptr;
}

void XConsoleNULL::UnregisterVariable(const char* sVarName)
{
    X_UNUSED(sVarName);
}

void XConsoleNULL::UnregisterVariable(ICVar* pVar)
{
    X_UNUSED(pVar);
}

void XConsoleNULL::RegisterCommand(const char* Name, ConsoleCmdFunc func, VarFlags Flags,
    const char* desc)
{
    X_UNUSED(Name);
    X_UNUSED(func);
    X_UNUSED(Flags);
    X_UNUSED(desc);
}

void XConsoleNULL::UnRegisterCommand(const char* Name)
{
    X_UNUSED(Name);
}

void XConsoleNULL::Exec(const char* command)
{
    X_UNUSED(command);
}

bool XConsoleNULL::LoadAndExecConfigFile(const char* fileName)
{
    X_UNUSED(fileName);
    return true;
}

/*
void XConsoleNULL::ConfigExec(const char* command)
{
X_UNUSED(command);

}*/

// Loggging
void XConsoleNULL::addLineToLog(const char* pStr, uint32_t length)
{
    X_UNUSED(pStr);
    X_UNUSED(length);
}

int XConsoleNULL::getLineCount(void) const
{
    return 0;
}
// ~Loggging

X_NAMESPACE_END
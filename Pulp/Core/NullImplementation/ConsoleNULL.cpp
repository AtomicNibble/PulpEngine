#include "stdafx.h"
#include "ConsoleNULL.h"

X_NAMESPACE_BEGIN(core)


XConsoleNULL::XConsoleNULL()
{

}

XConsoleNULL::~XConsoleNULL()
{

}

void XConsoleNULL::Startup(ICore* pCore, bool basic)
{
	X_UNUSED(pCore);
	X_UNUSED(basic);
}


void XConsoleNULL::RegisterCommnads(void)
{

}

void XConsoleNULL::ShutDown(void)
{

}

void XConsoleNULL::SaveChangedVars(void)
{

}

void XConsoleNULL::unregisterInputListener(void)
{

}

void XConsoleNULL::freeRenderResources(void)
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

ICVar* XConsoleNULL::RegisterString(const char* Name, const char* Value, int Flags,
	const char* desc)
{
	X_UNUSED(Name);
	X_UNUSED(Value);
	X_UNUSED(Flags);
	X_UNUSED(desc);
	return nullptr;
}

ICVar* XConsoleNULL::RegisterInt(const char* Name, int Value, int Min, int Max,
	int Flags, const char* desc)
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
	int flags, const char* desc)
{
	X_UNUSED(Name);
	X_UNUSED(Value);
	X_UNUSED(Min);
	X_UNUSED(Max);
	X_UNUSED(flags);
	X_UNUSED(desc);
	return nullptr;
}


ICVar* XConsoleNULL::ConfigRegisterString(const char* Name, const char* Value, int flags,
	const char* desc)
{
	X_UNUSED(Name);
	X_UNUSED(Value);
	X_UNUSED(flags);
	X_UNUSED(desc);
	return nullptr;
}

ICVar* XConsoleNULL::ConfigRegisterInt(const char* Name, int Value, int Min, int Max,
	int flags, const char* desc)
{
	X_UNUSED(Name);
	X_UNUSED(Value);
	X_UNUSED(Min);
	X_UNUSED(Max);
	X_UNUSED(flags);
	X_UNUSED(desc);
	return nullptr;
}

ICVar* XConsoleNULL::ConfigRegisterFloat(const char* Name, float Value, float Min,
	float Max, int flags, const char* desc)
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
	float Min, float Max, int flags, const char* desc)
{
	X_UNUSED(name);
	X_UNUSED(src);
	X_UNUSED(defaultvalue);
	X_UNUSED(Min);
	X_UNUSED(Max);
	X_UNUSED(flags);
	X_UNUSED(desc);

	return nullptr;
}

ICVar* XConsoleNULL::Register(const char* name, int* src, int defaultvalue,
	int Min, int Max, int flags, const char* desc)
{
	X_UNUSED(name);
	X_UNUSED(src);
	X_UNUSED(defaultvalue);
	X_UNUSED(Min);
	X_UNUSED(Max);
	X_UNUSED(flags);
	X_UNUSED(desc);

	return nullptr;
}

ICVar* XConsoleNULL::Register(const char* name, Color* src, Color defaultvalue,
	int flags, const char* desc)
{
	X_UNUSED(name);
	X_UNUSED(src);
	X_UNUSED(defaultvalue);
	X_UNUSED(flags);
	X_UNUSED(desc);

	return nullptr;
}

ICVar* XConsoleNULL::Register(const char* name, Vec3f* src, Vec3f defaultvalue,
	int flags, const char* desc)
{
	X_UNUSED(name);
	X_UNUSED(src);
	X_UNUSED(defaultvalue);
	X_UNUSED(flags);
	X_UNUSED(desc);

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


void XConsoleNULL::RegisterCommand(const char* Name, ConsoleCmdFunc func, int Flags,
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
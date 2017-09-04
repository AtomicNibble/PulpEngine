#include "EngineCommon.h"


X_NAMESPACE_BEGIN(core)

Assert::Assert( const SourceInfo& sourceInfo, const char* fmt, ... ) :
	sourceInfo_( sourceInfo )
{
	va_list ap;
	va_start( ap, fmt );

	if (gEnv)
	{
		if (gEnv->pLog) {
			gEnv->pLog->Assert(sourceInfo, fmt, ap);
		}
		if (gEnv->pCore) {
			gEnv->pCore->OnAssert(sourceInfo);
		}
	}

	if(!gEnv || !gEnv->pLog)
	{
#if X_PLATFORM_WIN32
		core::StackString<2048> temp;
		temp.append("ASSERT: ");
		temp.appendFmt(fmt, ap);

		wchar_t wTxt[2048] = { 0 };
		strUtil::Convert(temp.c_str(), wTxt);

		::OutputDebugStringW(wTxt);
#endif	

		// shieeeeet
		X_BREAKPOINT;
	}

	va_end( ap );
}



Assert& Assert::Variable(const char* const name, bool var)
{
	Dispatch( sourceInfo_, name, "%s", var ? "true" : "false" );
	return *this;
}

/// Dispatches the name and value of a char variable.
Assert& Assert::Variable(const char* const name, char var)
{
	Dispatch( sourceInfo_, name, "%c", var );
	return *this;
}

/// Dispatches the name and value of a signed char variable.
Assert& Assert::Variable(const char* const name, signed char var)
{
	Dispatch( sourceInfo_, name, "%c", var );
	return *this;
}

/// Dispatches the name and value of an unsigned char variable.
Assert& Assert::Variable(const char* const name, unsigned char var)
{
	Dispatch( sourceInfo_, name, "%c", var );
	return *this;
}

/// Dispatches the name and value of a short variable.
Assert& Assert::Variable(const char* const name, short var)
{
	Dispatch( sourceInfo_, name, "%i", var );
	return *this;
}

/// Dispatches the name and value of an unsigned short variable.
Assert& Assert::Variable(const char* const name, unsigned short var)
{
	Dispatch( sourceInfo_, name, "%u", var );
	return *this;
}

/// Dispatches the name and value of an int variable.
Assert& Assert::Variable(const char* const name, int var)
{
	Dispatch( sourceInfo_, name, "%i", var );
	return *this;
}

/// Dispatches the name and value of an unsigned int variable.
Assert& Assert::Variable(const char* const name, unsigned int var)
{
	Dispatch( sourceInfo_, name, "%u", var );
	return *this;
}

/// Dispatches the name and value of a long variable.
Assert& Assert::Variable(const char* const name, long var)
{
	Dispatch( sourceInfo_, name, "%i", var );
	return *this;
}

/// Dispatches the name and value of an unsigned long variable.
Assert& Assert::Variable(const char* const name, unsigned long var)
{
	Dispatch( sourceInfo_, name, "%u", var );
	return *this;
}

/// Dispatches the name and value of a long long variable.
Assert& Assert::Variable(const char* const name, long long var)
{
	Dispatch( sourceInfo_, name, "%11i", var );
	return *this;
}

/// Dispatches the name and value of an unsigned long long variable.
Assert& Assert::Variable(const char* const name, unsigned long long var)
{
	Dispatch( sourceInfo_, name, "%11u", var );
	return *this;
}

/// Dispatches the name and value of a float variable.
Assert& Assert::Variable(const char* const name, float var)
{
	Dispatch( sourceInfo_, name, "%f", var );
	return *this;
}

/// Dispatches the name and value of a double variable.
Assert& Assert::Variable(const char* const name, double var)
{
	Dispatch( sourceInfo_, name, "%9.7lf", var );
	return *this;
}

/// Dispatches the name and value of a string literal/string.
Assert& Assert::Variable(const char* const name, const char* const var )
{
	Dispatch( sourceInfo_, name, "'%s'", var );
	return *this;
}

void Assert::Dispatch(const SourceInfo& sourceInfo, const char* const name, const char* format, ...)
{
	X_VALIST_START(format)

	core::StackString256 value;
	value.setFmt(format, args);

	X_VALIST_END

	DispatchInternal(sourceInfo, name, value.c_str());
}



void Assert::DispatchInternal(const SourceInfo& sourceInfo, const char* const name, const char* pValue)
{
	if (gEnv)
	{
		if (gEnv->pLog) {
			gEnv->pLog->AssertVariable(sourceInfo, "Variable %s = %s", name, pValue);
		}
		if (gEnv->pCore) {
			gEnv->pCore->OnAssertVariable(sourceInfo);
		}
		else
		{
			X_BREAKPOINT;
		}

	}
}


X_NAMESPACE_END
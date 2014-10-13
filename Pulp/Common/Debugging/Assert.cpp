#include "EngineCommon.h"

#include <String\StackString.h>

X_NAMESPACE_BEGIN(core)

Assert::Assert( const SourceInfo& sourceInfo, const char* fmt, ... ) 
: m_sourceInfo( sourceInfo )
{
	va_list ap;
	va_start( ap, fmt );

//	core::logDispatch::Assert( sourceInfo, fmt, ap );
//	core::assertionDispatch::Assert( sourceInfo );
	if (gEnv)
	{
		if (gEnv->pLog)
			gEnv->pLog->Assert(sourceInfo, fmt, ap);
		if (gEnv->pCore)
			gEnv->pCore->OnAssert(sourceInfo);
	}
	else
	{
			// shieeeeet
#if X_DEBUG
		core::StackString<2048> temp;
		temp.append("ASSERT: ");
		temp.appendFmt(fmt, ap);

		::OutputDebugString(temp.c_str());

		X_BREAKPOINT;
#endif
		
	}

	va_end( ap );
}



Assert& Assert::Variable(const char* const name, bool var)
{
	Dispatch( m_sourceInfo, "Variable %s = %s", name, var ? "true" : "false" );
	return *this;
}

/// Dispatches the name and value of a char variable.
Assert& Assert::Variable(const char* const name, char var)
{
	Dispatch( m_sourceInfo, "Variable %s = %c", name, var );
	return *this;
}

/// Dispatches the name and value of a signed char variable.
Assert& Assert::Variable(const char* const name, signed char var)
{
	Dispatch( m_sourceInfo, "Variable %s = %c", name, var );
	return *this;
}

/// Dispatches the name and value of an unsigned char variable.
Assert& Assert::Variable(const char* const name, unsigned char var)
{
	Dispatch( m_sourceInfo, "Variable %s = %c", name, var );
	return *this;
}

/// Dispatches the name and value of a short variable.
Assert& Assert::Variable(const char* const name, short var)
{
	Dispatch( m_sourceInfo, "Variable %s = %i", name, var );
	return *this;
}

/// Dispatches the name and value of an unsigned short variable.
Assert& Assert::Variable(const char* const name, unsigned short var)
{
	Dispatch( m_sourceInfo, "Variable %s = %u", name, var );
	return *this;
}

/// Dispatches the name and value of an int variable.
Assert& Assert::Variable(const char* const name, int var)
{
	Dispatch( m_sourceInfo, "Variable %s = %i", name, var );
	return *this;
}

/// Dispatches the name and value of an unsigned int variable.
Assert& Assert::Variable(const char* const name, unsigned int var)
{
	Dispatch( m_sourceInfo, "Variable %s = %u", name, var );
	return *this;
}

/// Dispatches the name and value of a long variable.
Assert& Assert::Variable(const char* const name, long var)
{
	Dispatch( m_sourceInfo, "Variable %s = %i", name, var );
	return *this;
}

/// Dispatches the name and value of an unsigned long variable.
Assert& Assert::Variable(const char* const name, unsigned long var)
{
	Dispatch( m_sourceInfo, "Variable %s = %u", name, var );
	return *this;
}

/// Dispatches the name and value of a long long variable.
Assert& Assert::Variable(const char* const name, long long var)
{
	Dispatch( m_sourceInfo, "Variable %s = %11i", name, var );
	return *this;
}

/// Dispatches the name and value of an unsigned long long variable.
Assert& Assert::Variable(const char* const name, unsigned long long var)
{
	Dispatch( m_sourceInfo, "Variable %s = %11u", name, var );
	return *this;
}

/// Dispatches the name and value of a float variable.
Assert& Assert::Variable(const char* const name, float var)
{
	Dispatch( m_sourceInfo, "Variable %s = %f", name, var );
	return *this;
}

/// Dispatches the name and value of a double variable.
Assert& Assert::Variable(const char* const name, double var)
{
	Dispatch( m_sourceInfo, "Variable %s = %9.7lf", name, var );
	return *this;
}

/// Dispatches the name and value of a string literal/string.
Assert& Assert::Variable(const char* const name, const char* const var )
{
	Dispatch( m_sourceInfo, "Variable %s = '%s'", name, var );
	return *this;
}



X_NAMESPACE_END


// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
Assert& Assert::Variable(const char* const name, T* const var)
{
	Dispatch(sourceInfo_, "Variable %s = 0x%08p (pointer)", name, var);
	return *this;
}

// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
Assert& Assert::Variable(const char* const name, const T* const var)
{
	Dispatch(sourceInfo_, "Variable %s = 0x%08p (pointer)", name, var);
	return *this;
}

// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
Assert& Assert::Variable(const char* const, const T&)
{
	// empty implementation - users must provide a template specialization for their custom types
	return *this;
}

// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
void Assert::Dispatch( const SourceInfo& sourceInfo, const char* format, const char* const name, const T value )
{
	if (gEnv) 
	{
		if (gEnv->pLog) {
			gEnv->pLog->AssertVariable(sourceInfo, format, name, value);
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

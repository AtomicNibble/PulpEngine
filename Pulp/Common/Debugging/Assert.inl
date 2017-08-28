

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

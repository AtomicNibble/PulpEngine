

// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
Assert& Assert::Variable(const char* const name, T* const var)
{
	Dispatch(sourceInfo_, name, "0x%08p (pointer)", var);
	return *this;
}

// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
Assert& Assert::Variable(const char* const name, const T* const var)
{
	Dispatch(sourceInfo_, name, "0x%08p (pointer)", var);
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

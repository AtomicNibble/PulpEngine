
#pragma once
#ifndef X_CLASSINFO_H
#define X_CLASSINFO_H


X_NAMESPACE_BEGIN(core)

namespace compileTime
{

	template <typename T>
	struct IsTrivial
	{
		static const bool Value = std::is_trivial<T>::value;
	}; 

	template <typename T>
	struct HasTrivialCon
	{
		static const bool Value = std::has_trivial_constructor<T>::value;
	}; 

	template <typename T>
	struct HasTrivialCopy
	{
		static const bool Value = std::has_trivial_copy<T>::value;
	}; 

	template <typename T>
	struct HasTrivialDefaultCon
	{
		static const bool Value = std::has_trivial_default_constructor<T>::value;
	}; 


	template <typename T>
	struct HasVirtualDestructor
	{
		static const bool Value = std::has_virtual_destructor<T>::value;
	}; 


	template <typename T>
	struct IsAbstract
	{
		static const bool Value = std::is_abstract<T>::value;
	}; 


	template <typename T>
	struct IsPolymorphic
	{
		static const bool Value = std::is_polymorphic<T>::value;
	}; 

}

X_NAMESPACE_END


#endif // X_CLASSINFO_H


#pragma once
#ifndef X_ISPOINTER_H_
#define X_ISPOINTER_H_


X_NAMESPACE_BEGIN(core)

namespace compileTime
{

	template <typename T>
	struct IsPointer
	{
		static const bool Value = std::is_pointer<T>::value;
	}; 
}

X_NAMESPACE_END


#endif // X_ISPOINTER_H_

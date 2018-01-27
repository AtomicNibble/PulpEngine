#pragma once

#ifndef X_COMPILER_H
#define X_COMPILER_H


#define X_ABSTRACT									abstract
#define X_OVERRIDE									override
#define X_FINAL										override final 

#define X_PRAGMA(pragma)							__pragma(pragma)
#define X_PUSH_WARNING_LEVEL(level)					X_PRAGMA(warning(push, level))
#define X_POP_WARNING_LEVEL							X_PRAGMA(warning(pop))
#define X_DISABLE_WARNING(number)					X_PRAGMA(warning(disable:number))
#define X_ENABLE_WARNING(number)					X_PRAGMA(warning(default:number))
#define X_INTRINSIC(func)							X_PRAGMA(intrinsic(func))

#define X_PRAGMA_DIAG(x)							
#define X_DISABLE_WARNING_DIAG(name)				
#define X_WARNING_DIAG_PUSH							
#define X_WARNING_DIAG_POP							


#define X_DISABLE_EMPTY_FILE_WARNING				namespace { char NoEmptyFileDummy##__LINE__; }

#define X_RESTRICT									__restrict
#define X_RESTRICT_RV								__declspec(restrict)
#define X_NO_ALIAS									__declspec(noalias)
#define X_UNUSED_IMPL(symExpr, n)					, (void)(symExpr)
#define X_UNUSED(...)								(void)(true) X_PP_EXPAND_ARGS X_PP_PASS_ARGS(X_UNUSED_IMPL, __VA_ARGS__)
#define X_ALIGNED_SYMBOL(symbol, alignment)			__declspec(align(alignment)) symbol
#define X_ALIGN_OF(type)							__alignof(type)
#define X_INLINE									__forceinline
#define X_NO_INLINE									__declspec(noinline)
#define X_HINT(hint)								__assume(hint)
#define X_RETURN_ADDRESS()							_ReturnAddress()
#define X_FORCE_SYMBOL_LINK(symbolName)				X_PRAGMA(comment(linker, X_PP_JOIN("/include:", symbolName)))
#define X_LINK_LIB(libName)							X_PRAGMA(comment(lib, libName))
#define X_LINK_ENGINE_LIB(libName)					X_PRAGMA(comment(lib, X_ENGINE_OUTPUT_PREFIX libName))
#define X_MULTILINE_MACRO_BEGIN						X_DISABLE_WARNING(4127) do {
#define X_MULTILINE_MACRO_END						} while (0) X_ENABLE_WARNING(4127)
#define X_IMPORT									__declspec(dllimport)
#define X_EXPORT									__declspec(dllexport)
#define X_OFFSETOF(s,m)								offsetof(s,m)
#define X_ARRAY_SIZE(x)								((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define X_NO_SWITCH_DEFAULT							X_HINT(0)


#define X_PACK_PUSH(val)							X_PRAGMA(pack(push,val))
#define X_PACK_POP									X_PRAGMA(pack(pop))

#define X_ENSURE_GE(val1,val2,msg)					static_assert(val1 >= val2, msg);
#define X_ENSURE_LE(val1,val2,msg)					static_assert(val1 <= val2, msg);

#define COMPILER_BARRIER_R _ReadBarrier();
#define COMPILER_BARRIER_W _WriteBarrier();
#define COMPILER_BARRIER_RW _ReadWriteBarrier();

// _ReturnAddress must be prototyped before use, and can only be used as an intrinsic
extern "C" void* _ReturnAddress(void); 
X_INTRINSIC(_ReturnAddress)


#if 1 // shows the current size(template args are wrote to log), so i don't have to check.

template <typename ToCheck,
	size_t ExpectedSize,
	size_t RealSize = sizeof(ToCheck)>
struct check_size_static : std::true_type
{
	static_assert(ExpectedSize == RealSize, "type has a incorrect size");
};


#define X_ENSURE_SIZE(type,size) \
	static_assert(check_size_static<type, size>::value, "Size check fail");

#else 
	 #define X_ENSURE_SIZE(type,size)	static_assert(sizeof(type)==size, #type " is not " #size " bytes in size")
#endif


#endif // ! X_COMPILER_H

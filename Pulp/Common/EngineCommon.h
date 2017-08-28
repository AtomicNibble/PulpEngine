#pragma once

#ifndef _X_ENGINE_COMMON_INCLUDES_H
#define _X_ENGINE_COMMON_INCLUDES_H

#include <Core\Configuration.h>
#include <Core\Compiler.h>
#include <Core\Platform.h>

#include <Platform\Types.h>

X_DISABLE_WARNING(4714) // __forceinline not inlined.

// some std includes.
#include <limits>

// Util
#include <util\UserLiterals.h>
#include <Util\ClassMacros.h>
#include <Util\NamespaceMacros.h>
#include <Util\GenericUtil.h>
#include <Util\HelperMacros.h>
#include <Util\SourceInfo.h>
#include <Util\SourceInfoMacros.h>
#include <Util\Flags.h>
#include <Util\FlagsMacros.h>
#include <Util\EnumMacros.h>
#include <Util\BitUtil.h>
#include <Util\PointerUtil.h>
#include <Util\EndianUtil.h>

#include <Util\SmartPointer.h>


// preprocessor
#include <Prepro/PreproCommaIf.h>
#include <Prepro/PreproDecrement.h>
#include <Prepro/PreproExpandArgs.h>
#include <Prepro/PreproHasComma.h>
#include <Prepro/PreproIf.h>
#include <Prepro/PreproIncrement.h>
#include <Prepro/PreproIsEmpty.h>
#include <Prepro/PreproJoin.h>
#include <Prepro/PreproList.h>
#include <Prepro/PreproNumArgs.h>
#include <Prepro/PreproPassArgs.h>
#include <Prepro/PreproRepeat.h>
#include <Prepro/PreproStringize.h>
#include <Prepro/PreproToBool.h>
#include <Prepro/PreproUniqueName.h>


// compile time
#include <CompileTime/ExtractCount.h>
#include <CompileTime/ExtractType.h>
#include <CompileTime/IntToType.h>
#include <CompileTime/IsPOD.h>
#include <CompileTime/IsPointer.h>
#include <CompileTime/ClassInfo.h>

// debugging
#include <Debugging\DebuggerConnection.h>
#include <Debugging\DebuggerBreakpoint.h>
#include <Debugging\DebuggerBreakpointMacros.h>
#include <Debugging/Assert.h>
#include <Debugging/AssertMacros.h>

// traits
#include <Traits/FunctionTraits.h>
#include <Traits/MemberFunctionTraits.h>

// casts
#include <Casts/safe_static_cast.h>
#include <Casts/union_cast.h>


// Common used interface 
#include <IProfile.h>
#include <IEngineSysBase.h>
#include <ICore.h>


// threading
#include <Threading\Atomic.h>
#include <Threading\AtomicInt.h>
#include <Threading\CriticalSection.h>
#include <Threading\ConditionVariable.h>
#include <Threading\Semaphore.h>
#include <Threading\Spinlock.h>
#include <Threading\ThreadLocalStorage.h>
#include <Threading\Thread.h>


#include <Profile\ProfilerTypes.h>

// Memory
#include <Memory\NewAndDelete.h>
#include <Memory\NewAndDeleteMacros.h>
#include <Memory\HeapArea.h>
#include <Memory\MemoryArena.h>
#include <Memory\MemUtil.h>

// defaults.
#include <Memory\AllocationPolicies\MallocFreeAllocator.h>
#include <Memory\BoundsCheckingPolicies\SimpleBoundsChecking.h>
#include <Memory\BoundsCheckingPolicies\NoBoundsChecking.h>
#include <Memory\MemoryTaggingPolicies\SimpleMemoryTagging.h>
#include <Memory\MemoryTaggingPolicies\NoMemoryTagging.h>
#include <Memory\MemoryTrackingPolicies\SimpleMemoryTracking.h>
#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\ThreadPolicies\SingleThreadPolicy.h>


// Conatiners
#include <Containers\BitStream.h>
#include <Containers\ByteStream.h>
#include <Containers\Stack.h>
#include <Containers\FixedStack.h>
#include <Containers\Freelist.h>
#include <Containers\FixedRingBuffer.h>
// #include <Containers\LinkedList.h>
// #include <Containers\LinkedListIntrusive.h>

// string
#include <String\FormatMacros.h>
#include <String\StrRef.h>
#include <String\StringUtil.h>
#include <String\Path.h>
#include <String\StackString.h>
#include <String\Xml.h>
#include <String\Json.h>

// Math
#include <Math\XMath.h>
#include <Math\XVector.h>
#include <Math\XMatrix.h>

#include <Math\XColor.h>

#include <Math\XRay.h>
#include <Math\XRect.h>
#include <Math\XPlane.h>
#include <Math\XSphere.h>
#include <Math\XFrustum.h>
#include <Math\XCamera.h>
#include <Math\XQuat.h>
#include <Math\XTransform.h>
#include <Math\XHalf.h>

#include <Random\MultiplyWithCarry.h>
#include <Random\XorShift.h>

#define _STDINT // fuck standard types!
#include <memory>




#endif // _X_ENGINE_COMMON_INCLUDES_H
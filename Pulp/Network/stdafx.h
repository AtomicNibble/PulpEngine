#pragma once

#include <EngineCommon.h>

#include "Memory\BoundsCheckingPolicies\NoBoundsChecking.h"
#include "Memory\MemoryTaggingPolicies\NoMemoryTagging.h"
#include "Memory\MemoryTrackingPolicies\NoMemoryTracking.h"
#include "Memory\ThreadPolicies\MultiThreadPolicy.h"

typedef core::MemoryArena<
    core::MallocFreeAllocator,
    core::MultiThreadPolicy<core::Spinlock>,
#if X_DEBUG
    core::SimpleBoundsChecking,
    core::SimpleMemoryTracking,
    core::SimpleMemoryTagging
#else
    core::NoBoundsChecking,
    core::NoMemoryTracking,
    core::NoMemoryTagging
#endif // !X_DEBUG
    >
    NetworkArena;

extern NetworkArena* g_NetworkArena;

#include <INetwork.h>
#include <NetMsgIds.h>

X_NAMESPACE_BEGIN(net)

namespace platform
{
#if X_PLATFORM_WIN32

#ifndef NEAR
#define NEAR
#endif

#ifndef FAR
#define FAR
#endif

#include <WinSock2.h>
#include <Ws2tcpip.h>

X_LINK_LIB("Ws2_32.lib");

#endif // X_PLATFORM_WIN32

} // namespace platform

typedef platform::SOCKET SocketHandle;

X_NAMESPACE_END

#include <Containers\FixedBitStream.h>

#include "Util\Config.h"
#include "Util\SystemAddressEx.h"
#include "Util\Constants.h"
#include "Util\LastError.h"

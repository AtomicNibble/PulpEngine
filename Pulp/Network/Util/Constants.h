#pragma once


#ifdef INVALID_SOCKET
#undef INVALID_SOCKET
#endif // !INVALID_SOCKET

X_NAMESPACE_BEGIN(net)

static const uint32_t PROTO_VERSION_MAJOR = 1;
static const uint32_t PROTO_VERSION_MINOR = 0;


static const SocketHandle INVALID_SOCKET = SocketHandle(~0);

const SystemAdd UNASSIGNED_SYSTEM_ADDRESS;
const NetGUID UNASSIGNED_NET_GUID(0ull);

static const uint32_t MAX_INTERNAL_IDS = 8;
static const uint32_t MAX_ORDERED_STREAMS = 255;
static const uint32_t MAX_MTU_SIZE = 1492;
static const uint32_t MIN_MTU_SIZE = 400;

static const uint16_t UNDEFINED_PING = std::numeric_limits<uint16_t>::max();


X_NAMESPACE_END

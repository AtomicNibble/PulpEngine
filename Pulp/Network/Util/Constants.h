#pragma once


#ifdef INVALID_SOCKET
#undef INVALID_SOCKET
#endif // !INVALID_SOCKET

X_NAMESPACE_BEGIN(net)

static const uint8_t PROTO_VERSION_MAJOR = 1;
static const uint8_t PROTO_VERSION_MINOR = 0;


static const SocketHandle INVALID_SOCKET = SocketHandle(~0);

const SystemAdd UNASSIGNED_SYSTEM_ADDRESS;
const NetGUID UNASSIGNED_NET_GUID(0ull);

static const uint32_t MAX_INTERNAL_IDS = 8;
static const uint32_t MAX_MTU_SIZE = 1492; // 1472 
static const uint32_t MIN_MTU_SIZE = 400;
static const uint32_t UDP_HEADER_SIZE = 28;

// bumpinging this increases memory useage a tiny bit for each reliable datagram. making this 128 wouldbe fine tho.
// if we found we sent a lot of tiny packets.
static const uint32_t MAX_PACKETS_PER_DATAGRAM = 56; // 2bytes per entry + 16bytes 

// controlls the size of the resend buffer for each connection.
static const size_t REL_RESEND_BUF_LENGTH = 256;
static const size_t REL_DATAGRAM_HISTORY_LENGTH = 256;
static const size_t REL_MAX_RECIVE_HOLE = 16'384;
static const size_t REL_RECIVE_HOLE_SHRINK_THRESHOLD = 256; // recvice hole must be greater than this to be considered for shrink_to_fit.

static const uint16_t UNDEFINED_PING = std::numeric_limits<uint16_t>::max();


X_NAMESPACE_END

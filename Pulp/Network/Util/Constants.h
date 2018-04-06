#pragma once

#ifdef INVALID_SOCKET
#undef INVALID_SOCKET
#endif // !INVALID_SOCKET

X_NAMESPACE_BEGIN(net)

static const uint8_t PROTO_VERSION_MAJOR = 1;
static const uint8_t PROTO_VERSION_MINOR = 0;

static const SocketHandle INVALID_SOCKET = SocketHandle(~0);

const SystemAddressEx UNASSIGNED_SYSTEM_ADDRESS;
const NetGUID UNASSIGNED_NET_GUID(0ull);

static const uint32_t MAX_BANS = 64; // if need more bans than this, ill sort them so can check if banned quicker. this is basically a 'it's worth the effort now limit'.
static const uint32_t MAX_INTERNAL_IDS = 8;
static const uint32_t MAX_MTU_SIZE = 1492; // 1472
static const uint32_t MIN_MTU_SIZE = 400;
static const uint32_t UDP_HEADER_SIZE = 28;

// bumpinging this increases memory useage a tiny bit for each reliable datagram. making this 128 wouldbe fine tho.
// if we found we sent a lot of tiny packets.
static const uint32_t MAX_REL_PACKETS_PER_DATAGRAM = 56; // 2bytes per entry + 16bytes

// controlls the size of the resend buffer for each connection.
static const size_t REL_RESEND_BUF_LENGTH = 512;            // this controls how many packets can be on wire at once.
static const size_t REL_DATAGRAM_HISTORY_LENGTH = 512;      // this is how much datagram history we keep. this should not be less than REL_RESEND_BUF_LENGTH.
static const size_t REL_MAX_RECIVE_HOLE = 16'384;           // this should be above MAX_PACKETS_PER_DATAGRAM * REL_RESEND_BUF_LENGTH to prevent any packets been ignored if arrive super early as expensive of memory. (setting lower is fine)
static const size_t REL_RECIVE_HOLE_SHRINK_THRESHOLD = 256; // when recvice hole is greater than this it's considered for shrink_to_fit. helps rake memory back if you allow large recive hole.

static const uint16_t UNDEFINED_PING = std::numeric_limits<uint16_t>::max();

X_NAMESPACE_END

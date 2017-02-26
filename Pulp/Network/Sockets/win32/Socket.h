#pragma once


X_NAMESPACE_BEGIN(net)


X_DECLARE_ENUM(BindResult)(
	Success,
	FailedToBind,
	SendTestFailed
);

X_DECLARE_ENUM(SocketType)(
	Stream,				// stream	
	Dgram,				// datagram
	Raw,				// raw-protocol
	Rdm,				// reliably-deliverd
	SeqPacket,			// sequenced packet stream
	Invalid
);

// anyhing negative is error.
typedef int32_t SendResult;


struct BindParameters
{
	BindParameters();

	HostAddStr hostAdd;
	uint16_t port;
	SocketFamily::Enum socketFamily;
	SocketType::Enum socketType;

	bool nonBlockingSocket;
	bool IPHdrIncl;
	bool broadCast;
	bool _pad;
};

struct SendParameters
{
	X_INLINE SendParameters() : ttl(0) { }

	uint8_t* pData;
	int32_t length;
	int32_t ttl;
	SystemAdd systemAddress;
};

struct RecvData
{
	uint8_t data[MAX_MTU_SIZE];
	int32_t bytesRead;
	core::TimeVal timeRead;
	SystemAdd systemAdd;
};

class NetSocket
{
public:
	typedef core::FixedArray<SystemAdd, MAX_INTERNAL_IDS> SystemAddArr;

public:
	NetSocket();
	~NetSocket();

	BindResult::Enum bind(BindParameters& bindParameters);
	SendResult send(SendParameters& sendParameters);
	void recv(RecvData& dataOut);
	
	X_INLINE const SystemAdd& getBoundAdd(void) const;

public:
	static bool getMyIPs(SystemAddArr& addresses);

private:
	void setNonBlockingSocket(bool nonblocking);
	void setSocketOptions(void);
	void setBroadcastSocket(bool broadcast);
	void setIPHdrIncl(bool ipHdrIncl);

	void setTTL(IpVersion::Enum ipVer, int32_t ttl);
	bool getTTL(IpVersion::Enum ipVer, int32_t& ttl);

private:
	SocketType::Enum socketType_;
	SocketHandle socket_;
	SystemAdd boundAdd_;
};


X_NAMESPACE_END

#include "Socket.inl"
#pragma once


X_NAMESPACE_BEGIN(net)


X_DECLARE_ENUM(BindResult)(
	Success,
	FailedToBind,
	SendTestFailed
);

X_DECLARE_ENUM(SendResult)(
	Success
);

X_DECLARE_ENUM(SocketType)(
	Stream,				// stream	
	Dgram,				// datagram
	Raw,				// raw-protocol
	Rdm,				// reliably-deliverd
	SeqPacket,			// sequenced packet stream
	Invalid
);


struct BindParameters
{
	BindParameters();

	HostAddStr hostAdd;
	uint16_t port;
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


class NetSocket
{
public:
	NetSocket();
	~NetSocket();

	BindResult::Enum bind(BindParameters& bindParameters);
	SendResult::Enum send(SendParameters& sendParameters);
	
	X_INLINE const SystemAdd& getBoundAdd(void) const;

private:
	void setNonBlockingSocket(bool nonblocking);
	void setSocketOptions(void);
	void setBroadcastSocket(bool broadcast);
	void setIPHdrIncl(bool ipHdrIncl);

private:
	SocketType::Enum socketType_;
	SocketHandle socket_;
	SystemAdd boundAdd_;
};


X_NAMESPACE_END

#include "Socket.inl"
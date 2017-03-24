#pragma once


X_NAMESPACE_BEGIN(net)

class NetVars;

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

X_DECLARE_ENUM(RecvResult)(
	Success,
	ConnectionReset,
	Error
);

struct BindParameters
{
	BindParameters();

	HostStr hostAdd;
	Port port;
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

	X_INLINE void setData(core::BitStream& bs) {
		pData = bs.data();
		length = safe_static_cast<int32_t>(bs.sizeInBytes());
	}
	X_INLINE void setData(core::FixedBitStreamBase& bs) {
		pData = bs.data();
		length = safe_static_cast<int32_t>(bs.sizeInBytes());
	}

	uint8_t* pData;
	int32_t length;
	int32_t ttl;
	SystemAddressEx systemAddress;
};

class NetSocket;

struct RecvData
{
	uint8_t data[MAX_MTU_SIZE];
	int32_t bytesRead;
	core::TimeVal timeRead;
	SystemAddressEx systemAddress;
	NetSocket* pSrcSocket;
};

class NetSocket
{
public:
	typedef core::FixedArray<SystemAddressEx, MAX_INTERNAL_IDS> SystemAddArr;

public:
	NetSocket(NetVars& vars);
	NetSocket(NetSocket&& oth);
	~NetSocket();

	NetSocket& operator=(NetSocket&& oth);


	BindResult::Enum bind(BindParameters& bindParameters);
	bool sendSendTest(void);
	SendResult send(SendParameters& sendParameters);
	RecvResult::Enum recv(RecvData& dataOut);
	
	X_INLINE const SystemAddressEx& getBoundAdd(void) const;

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
	X_NO_COPY(NetSocket);
	X_NO_ASSIGN(NetSocket);

private:
	NetVars& vars_;

	SocketType::Enum socketType_;
	SocketHandle socket_;
	SystemAddressEx boundAdd_;
};


X_NAMESPACE_END

#include "Socket.inl"
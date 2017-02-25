#pragma once

X_NAMESPACE_BEGIN(net)


class XPeer : public IPeer
{
public:
	XPeer();
	~XPeer() X_FINAL;

	// IPeer

	StartupResult::Enum init(uint32_t maxConnections, SocketDescriptor* pSocketDescriptors,
		uint32_t socketDescriptorCount) X_FINAL;
	void shutdown(core::TimeVal blockDuration, uint8_t orderingChannel = 0,
		PacketPriority::Enum disconnectionNotificationPriority = PacketPriority::Low) X_FINAL;

	// connection api
	ConnectionAttemptResult::Enum connect(const char* pHost, Port remotePort,
		uint32_t connectionSocketIndex = 0, uint32_t sendConnectionAttemptCount = 12, uint32_t
		timeBetweenSendConnectionAttemptsMS = 500, core::TimeVal timeoutTime = core::TimeVal()) X_FINAL;
	void closeConnection(const AddressOrGUID target, bool sendDisconnectionNotification,
		uint8_t orderingChannel = 0, PacketPriority::Enum disconnectionNotificationPriority = PacketPriority::Low) X_FINAL;

	// connection util
	ConnectionState::Enum getConnectionState(const AddressOrGUID systemIdentifier) X_FINAL;
	void cancelConnectionAttempt(const ISystemAdd* pTarget) X_FINAL;


	uint32_t send(const char* pData, const size_t length, PacketPriority::Enum priority,
		PacketReliability::Enum reliability, uint8_t orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, uint32_t forceReceiptNumber = 0) X_FINAL;

	// connection limits
	void setMaximumIncomingConnections(uint16_t numberAllowed) X_FINAL;
	uint16_t getMaximumIncomingConnections(void) const X_FINAL;
	uint16_t numberOfConnections(void) const X_FINAL;

	// Ping 
	void ping(const ISystemAdd* pTarget) X_FINAL;
	bool ping(const char* pHost, uint16_t remotePort, bool onlyReplyOnAcceptingConnections,
		uint32_t connectionSocketIndex = 0) X_FINAL;


	int getAveragePing(const AddressOrGUID systemIdentifier) X_FINAL;
	int getLastPing(const AddressOrGUID systemIdentifier) const X_FINAL;
	int getLowestPing(const AddressOrGUID systemIdentifier) const X_FINAL;


	const NetGUID& getMyGUID(void) const X_FINAL;

	void setTimeoutTime(core::TimeVal time, const ISystemAdd* pTarget) X_FINAL;
	core::TimeVal getTimeoutTime(const ISystemAdd* pTarget = nullptr) X_FINAL;

	// MTU for a given system
	int getMTUSize(const ISystemAdd* pTarget = nullptr) X_FINAL;

	bool getStatistics(const ISystemAdd* pTarget, NetStatistics& stats) X_FINAL;

	// ~IPeer


private:
	NetGUID guid_;

	core::TimeVal defaultTimeOut_;
	int32_t defaultMTU_;
};



X_NAMESPACE_END


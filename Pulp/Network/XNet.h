#pragma once

#include "Vars\NetVars.h"

X_NAMESPACE_DECLARE(core,
struct ICVar;
struct IConsoleCmdArgs;
)

X_NAMESPACE_BEGIN(net)

class XPeer;
class XNet : public INet
{
	typedef core::FixedArray<XPeer*, MAX_PEERS> PeerArr;

public:
	XNet(core::MemoryArenaBase* arena);
	~XNet() X_FINAL;

	// INet
	void registerVars(void) X_FINAL;
	void registerCmds(void) X_FINAL;

	bool init(void) X_FINAL;
	void shutDown(void) X_FINAL;
	void release(void) X_FINAL;


	IPeer* createPeer(void) X_FINAL;
	void deletePeer(IPeer* pPeer) X_FINAL;

	bool systemAddressFromIP(const IPStr& ip, SystemAddress& out, IpVersion::Enum ipVersion = IpVersion::Any) const X_FINAL;
	bool systemAddressFromIP(const IPStr& ip, Port port, SystemAddress& out, IpVersion::Enum ipVersion = IpVersion::Any) const X_FINAL;

	bool systemAddressFromHost(const HostStr& host, SystemAddress& out, IpVersion::Enum ipVersion = IpVersion::Any) const X_FINAL;
	bool systemAddressFromHost(const HostStr& host, Port port, SystemAddress& out, IpVersion::Enum ipVersion = IpVersion::Any) const X_FINAL;
	
	const char* systemAddressToString(const SystemAddress& systemAddress, IPStr& strBuf, bool incPort = true) const X_FINAL;
	// ~INet

	static NetGUID generateGUID(void);

private:
	void Cmd_listRemoteSystems(core::IConsoleCmdArgs* pCmd);
	void Cmd_clearBans(core::IConsoleCmdArgs* pCmd);
	void Cmd_listBans(core::IConsoleCmdArgs* pCmd);
	void Cmd_addBan(core::IConsoleCmdArgs* pCmd);
	void Cmd_removeBan(core::IConsoleCmdArgs* pCmd);

private:
	core::MemoryArenaBase* arena_;
	
	PeerArr peers_;
	NetVars vars_;
};


X_NAMESPACE_END


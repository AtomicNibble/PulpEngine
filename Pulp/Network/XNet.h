#pragma once

#include "Vars\NetVars.h"

X_NAMESPACE_DECLARE(core,
                    struct ICVar;
                    struct IConsoleCmdArgs;)

X_NAMESPACE_DECLARE(core,
    namespace V2 {
        struct Job;
        class JobSystem;
    })

X_NAMESPACE_BEGIN(net)

class XPeer;
class XNet : public INet
{
    typedef core::FixedArray<XPeer*, MAX_PEERS> PeerArr;
    typedef core::FixedArray<SystemAddressEx, MAX_INTERNAL_IDS> SystemAddArr;

    X_NO_COPY(XNet);
    X_NO_ASSIGN(XNet);

public:
    XNet(core::MemoryArenaBase* arena);
    ~XNet() X_FINAL;

    // INet
    void registerVars(void) X_FINAL;
    void registerCmds(void) X_FINAL;

    bool init(void) X_FINAL;
    bool asyncInitFinalize(void) X_FINAL;
    void shutDown(void) X_FINAL;
    void release(void) X_FINAL;

    IPeer* createPeer(void) X_FINAL;
    void deletePeer(IPeer* pPeer) X_FINAL;

    bool systemAddressFromIP(const IPStr& ip, SystemAddress& out, IpVersion::Enum ipVersion = IpVersion::Any) const X_FINAL;
    bool systemAddressFromIP(const IPStr& ip, Port port, SystemAddress& out, IpVersion::Enum ipVersion = IpVersion::Any) const X_FINAL;

    bool systemAddressFromHost(const HostStr& host, SystemAddress& out, IpVersion::Enum ipVersion = IpVersion::Any) const X_FINAL;
    bool systemAddressFromHost(const HostStr& host, Port port, SystemAddress& out, IpVersion::Enum ipVersion = IpVersion::Any) const X_FINAL;

    bool systemAddressFromHost(const HostStr& host, SystemAddressResolveArr& out, IpVersion::Enum ipVersion = IpVersion::Any) const X_FINAL;

    const char* systemAddressToString(const SystemAddress& systemAddress, IPStr& strBuf, bool incPort = true) const X_FINAL;
    // ~INet

    static NetGUID generateGUID(void);

private:
    void asyncInit_Job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

private:
    bool populateIpList(void);

private:
    void Cmd_listLocalAddress(core::IConsoleCmdArgs* pCmd);
    void Cmd_listRemoteSystems(core::IConsoleCmdArgs* pCmd);
    void Cmd_clearBans(core::IConsoleCmdArgs* pCmd);
    void Cmd_listBans(core::IConsoleCmdArgs* pCmd);
    void Cmd_addBan(core::IConsoleCmdArgs* pCmd);
    void Cmd_removeBan(core::IConsoleCmdArgs* pCmd);
    void Cmd_resolveHost(core::IConsoleCmdArgs* pCmd);

private:
    core::MemoryArenaBase* arena_;
    core::V2::Job* pInitJob_;

    SystemAddArr ipList_;

    PeerArr peers_;
    NetVars vars_;
};

X_NAMESPACE_END

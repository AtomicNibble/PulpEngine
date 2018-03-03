#include "stdafx.h"
#include "XNet.h"
#include "XPeer.h"

#include "Util\LibaryStartup.h"
#include <Time\StopWatch.h>

#include <Time\DateStamp.h>
#include <ITimer.h>
#include <IConsole.h>
#include <Threading\JobSystem2.h>
#include <Util\Process.h>

#include <Hashing\sha1.h>

X_NAMESPACE_BEGIN(net)

XNet::XNet(core::MemoryArenaBase* arena) :
	arena_(arena),
	pInitJob_(nullptr)
{

}

XNet::~XNet()
{

}

// INet
void XNet::registerVars(void)
{
	vars_.registerVars();
}

void XNet::registerCmds(void)
{
	ADD_COMMAND_MEMBER("net_left_local_add", this, XNet, &XNet::Cmd_listLocalAddress, core::VarFlag::SYSTEM,
		"Lists local addresses");
	ADD_COMMAND_MEMBER("net_list_remotes", this, XNet, &XNet::Cmd_listRemoteSystems, core::VarFlag::SYSTEM,
		"List all the connected systems for each peer. <verbose>");
	ADD_COMMAND_MEMBER("net_bans_clear", this, XNet, &XNet::Cmd_clearBans, core::VarFlag::SYSTEM,
		"Clears all bans");
	ADD_COMMAND_MEMBER("net_bans_list", this, XNet, &XNet::Cmd_listBans, core::VarFlag::SYSTEM,
		"Lists all bans");
	ADD_COMMAND_MEMBER("net_bans_add", this, XNet, &XNet::Cmd_addBan, core::VarFlag::SYSTEM,
		"Add a ban. <address>, <timeoutMS> (0=unlimted)");
	ADD_COMMAND_MEMBER("net_bans_remove", this, XNet, &XNet::Cmd_removeBan, core::VarFlag::SYSTEM,
		"Removes a ban if found. <address>");

	ADD_COMMAND_MEMBER("net_resolve", this, XNet, &XNet::Cmd_resolveHost, core::VarFlag::SYSTEM,
		"Resolves a given host, result is logged. <host>, <forceIpVersion(ipv4|ipv6)>");
}

bool XNet::init(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_LOG0("Net", "Starting");
	X_PROFILE_NO_HISTORY_BEGIN("NetInit", core::profiler::SubSys::NETWORK);


	if (!gEnv->pJobSys) 
	{
		if (!PlatLib::addRef()) {
			return false;
		}

		if (!populateIpList()) {
			return false;
		}
	}
	else
	{
		pInitJob_ = gEnv->pJobSys->CreateMemberJobAndRun<XNet>(this, &XNet::asyncInit_Job, nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::NETWORK));
	}

	return true;
}

bool XNet::asyncInitFinalize(void)
{
	if (pInitJob_) {
		gEnv->pJobSys->Wait(pInitJob_);
		pInitJob_ = nullptr;
	}

	if (!PlatLib::isStarted()) {
		return false;
	}

	if (ipList_.isEmpty()) {
		X_LOG0("Net", "Failed to get local addresses");
		return false;
	}

	return true;
}

void XNet::shutDown(void)
{
	X_LOG0("Net", "Shutting Down");

	for (auto* pPeer : peers_)
	{
		X_DELETE(pPeer, arena_);
	}

	PlatLib::deRef();
}

void XNet::release(void)
{
	X_DELETE(this, g_NetworkArena);
}


IPeer* XNet::createPeer(void)
{
	X_ASSERT(!pInitJob_, "Async init not finalized")();

	if (peers_.size() == peers_.capacity()) {
		X_ERROR("Net", "Failed to create peer, reached max peer count: %" PRIu32, MAX_PEERS);
		return nullptr;
	}

	XPeer* pPeer = X_NEW(XPeer, arena_, "Peer")(vars_, ipList_, arena_);
	peers_.append(pPeer);
	return pPeer;
}

void XNet::deletePeer(IPeer* pIPeer)
{
	if (!pIPeer) {
		return;
	}

	XPeer* pPeer = static_cast<XPeer*>(pIPeer);

	auto idx = peers_.find(pPeer);
	X_ASSERT(idx != PeerArr::invalid_index, "Failed to find peer instance")();
	peers_.removeIndex(idx);
	X_DELETE(pPeer, arena_);
}

bool XNet::systemAddressFromIP(const IPStr& ip, SystemAddress& out, IpVersion::Enum ipVersion) const
{
	SystemAddressEx& sa = static_cast<SystemAddressEx&>(out);
	if (!sa.fromIP(ip, SystemAddressEx::PORT_DELINEATOR, ipVersion)) {
		return false;
	}

	return true;
}

bool XNet::systemAddressFromIP(const IPStr& ip, Port port, SystemAddress& out, IpVersion::Enum ipVersion) const
{
	SystemAddressEx& sa = static_cast<SystemAddressEx&>(out);
	if (!sa.fromIP(ip, port, ipVersion)) {
		return false;
	}

	return true;
}


bool XNet::systemAddressFromHost(const HostStr& host, SystemAddress& out, IpVersion::Enum ipVersion) const
{
	SystemAddressEx& sa = static_cast<SystemAddressEx&>(out);
	if (!sa.fromHost(host, SystemAddressEx::PORT_DELINEATOR, ipVersion)) {
		return false;
	}

	return true;
}

bool XNet::systemAddressFromHost(const HostStr& host, Port port, SystemAddress& out, IpVersion::Enum ipVersion) const
{
	SystemAddressEx& sa = static_cast<SystemAddressEx&>(out);
	if (!sa.fromHost(host, port, ipVersion)) {
		return false;
	}

	return true;
}

const char* XNet::systemAddressToString(const SystemAddress& systemAddress, IPStr& strBuf, bool incPort) const
{
	const SystemAddressEx& sa = static_cast<const SystemAddressEx&>(systemAddress);

	return sa.toString(strBuf, incPort);
}

// ~INet

NetGUID XNet::generateGUID(void)
{
	// this needs to be unique as much as possible.
	// even if game started with same seed needs t obe diffrent so clients are still unique.
	core::TimeVal now = gEnv->pTimer->GetTimeNowReal();
	core::DateStamp date = core::DateStamp::GetSystemDate();

	core::Hash::SHA1 sha1;
	sha1.update(date);
	sha1.update(now.GetValue());
	sha1.update(core::Thread::GetCurrentID());
	sha1.update(core::Process::GetCurrentID());

	auto digest = sha1.finalize();

	uint64_t val = digest.data[0];
	val <<= 32;
	val |= digest.data[1];

	return NetGUID(val);
}

void XNet::asyncInit_Job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);
	X_UNUSED(pData);

	if (!PlatLib::addRef()) {
		return;
	}

	populateIpList();
}

bool XNet::populateIpList(void)
{
	if (!NetSocket::getMyIPs(ipList_)) {
		return false;
	}

	return true;
}


// -------------------------


void XNet::Cmd_listLocalAddress(core::IConsoleCmdArgs* pCmd)
{
	X_UNUSED(pCmd);

	if (peers_.empty()) {
		return;
	}

	peers_.front()->listLocalAddress();
}

void XNet::Cmd_listRemoteSystems(core::IConsoleCmdArgs* pCmd)
{
	bool verbose = false;

	if (pCmd->GetArgCount() == 2)
	{
		const char* pVerboseStr = pCmd->GetArg(1);
		if (core::strUtil::IsNumeric(pVerboseStr)) {
			verbose = core::strUtil::StringToInt<int32_t>(pVerboseStr) == 1;
		}
		else {
			verbose = core::strUtil::IsEqualCaseInsen(pVerboseStr, "true");
		}
	}


	X_LOG0("Console", "----------- ^8Remote Systems^7 -----------");

	int32_t idx = 0;
	for (auto* pPeer : peers_)
	{
		X_LOG0("Net", "Peer%" PRIi32" remote systems", idx++);
		X_LOG_BULLET;
		pPeer->listRemoteSystems(verbose);
	}

	X_LOG0("Console", "--------- ^8Remote Systems End^7 ---------");
}

void XNet::Cmd_clearBans(core::IConsoleCmdArgs* pCmd)
{
	X_UNUSED(pCmd);

	for (auto* pPeer : peers_)
	{
		pPeer->clearBanList();
	}
}

void XNet::Cmd_listBans(core::IConsoleCmdArgs* pCmd)
{
	X_UNUSED(pCmd);

	int32_t idx = 0;
	for (auto* pPeer : peers_)
	{
		X_LOG0("Net", "Peer%" PRIi32" bans", idx++);
		X_LOG_BULLET;
		pPeer->listBans();
	}
}

void XNet::Cmd_addBan(core::IConsoleCmdArgs* pCmd)
{
	if (pCmd->GetArgCount() != 3) {
		X_WARNING("Net", "net_bans_add <address>, <timeoutMS>");
		return;
	}

	const char* pIP = pCmd->GetArg(1);
	if (core::strUtil::strlen(pIP) > IPStr::BUF_SIZE) {
		X_WARNING("Net", "Ip is too long");
		return;
	}

	IPStr ip(pIP);

	if (!SystemAddressEx::isValidIP(ip, IpVersion::Any)) {
		X_ERROR("Net", "Can't add ban for \"%s\" it's a invalid address", ip.c_str());
		return;
	}

	core::TimeVal timeout;

	int32_t timeoutMS = core::strUtil::StringToInt<int32_t>(pCmd->GetArg(2));
	if (timeoutMS < 0) {
		timeoutMS = 0;
	}
	
	timeout.SetMilliSeconds(timeoutMS);

	for (auto* pPeer : peers_)
	{
		pPeer->addToBanList(ip, timeout);
	}
}

void XNet::Cmd_removeBan(core::IConsoleCmdArgs* pCmd)
{
	if (pCmd->GetArgCount() != 2) {
		X_WARNING("Net", "net_bans_remove <address>");
		return;
	}

	const char* pIP = pCmd->GetArg(1);
	if (core::strUtil::strlen(pIP) > IPStr::BUF_SIZE) {
		X_WARNING("Net", "Ip is too long");
		return;
	}

	IPStr ip(pIP);
	if (!SystemAddressEx::isValidIP(ip, IpVersion::Any)) {
		X_ERROR("Net", "Can't add ban for \"%s\" it's a invalid address", ip.c_str());
		return;
	}

	for (auto* pPeer : peers_)
	{
		pPeer->removeFromBanList(ip);
	}
}

void XNet::Cmd_resolveHost(core::IConsoleCmdArgs* pCmd)
{
	if (pCmd->GetArgCount() < 2) {
		X_WARNING("Net", "net_resolve <host>, <forceIpVersion(ipv4|ipv6)>");
		return;
	}

	const char* pHost = pCmd->GetArg(1);
	if (core::strUtil::strlen(pHost) > HostStr::BUF_SIZE) {
		X_WARNING("Net", "Host name is too long");
		return;
	}

	IpVersion::Enum ipVersion = IpVersion::Any;
	if (pCmd->GetArgCount() > 2)
	{
		const char* pIpVersion = pCmd->GetArg(2);
		if (core::strUtil::IsEqualCaseInsen(pIpVersion, "ipv4"))
		{
			ipVersion = IpVersion::Ipv4;
		}
		else if (core::strUtil::IsEqualCaseInsen(pIpVersion, "ipv6"))
		{
			ipVersion = IpVersion::Ipv6;
		}
		else
		{
			X_WARNING("Net", "net_resolve: Unknown ipVersion defaulting to Any");
		}
	}

	HostStr hostStr(pHost);
	SystemAddressEx address;

	core::StopWatch timer;

	if (!systemAddressFromHost(hostStr, address, ipVersion)) {
		X_WARNING("Net", "Failed to resolve");
		return;
	}

	IPStr ipStr;
	address.toString(ipStr);

	X_LOG0("Net", "Host: \"%s\" address: \"%s\" ^6%gms", hostStr.c_str(), ipStr.c_str(), timer.GetMilliSeconds());
}

X_NAMESPACE_END


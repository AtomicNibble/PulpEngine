#include "stdafx.h"
#include "XNet.h"
#include "XPeer.h"

#include "Util\LibaryStartup.h"

#include <Time\DateStamp.h>
#include <ITimer.h>
#include <IConsole.h>

#include <Hashing\sha1.h>

X_NAMESPACE_BEGIN(net)

XNet::XNet(core::MemoryArenaBase* arena) :
	arena_(arena)
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
	ADD_COMMAND_MEMBER("net_list_remotes", this, XNet, &XNet::listRemoteSystems, core::VarFlag::SYSTEM,
		"List all the connected systems for each peer");

}

bool XNet::init(void)
{
	X_LOG0("Net", "Starting");


	if (!PlatLib::addRef()) {
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

}


IPeer* XNet::createPeer(void)
{
	if (peers_.size() == peers_.capacity()) {
		return nullptr;
	}

	XPeer* pPeer = X_NEW(XPeer, arena_, "Peer")(vars_, arena_);
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

ISystemAdd* XNet::createSysAddress(const char* pAddressStr)
{
	return X_NEW(SystemAdd, arena_, "SysAdd")(pAddressStr);
}

ISystemAdd* XNet::createSysAddress(const char* pAddressStr, uint16_t port)
{
	return X_NEW(SystemAdd, arena_, "SysAdd")(pAddressStr, port);
}
// ~INet

NetGUID XNet::generateGUID(void)
{
	// this needs to be unique as much as possible.
	// even if game started with same seed needs t obe diffrent so clients are still unique.
	core::TimeVal now = gEnv->pTimer->GetTimeNowReal();
	core::DateStamp date = core::DateStamp::GetSystemDate();

	core::Hash::SHA1 sha1;
	sha1.Init();
	sha1.update(date);
	sha1.update(now.GetValue());
	auto digest = sha1.finalize();

	uint64_t val = digest.data[0];
	val <<= 32;
	val |= digest.data[1];

	return NetGUID(val);
}

void XNet::listRemoteSystems(core::IConsoleCmdArgs* pCmd)
{
	X_UNUSED(pCmd);

	int32_t idx = 0;
	for (auto* pPeer : peers_)
	{
		X_LOG0("Net", "Peer%" PRIi32" remote systems", idx++);
		X_LOG_BULLET;
		pPeer->listRemoteSystems();
	}
}

X_NAMESPACE_END


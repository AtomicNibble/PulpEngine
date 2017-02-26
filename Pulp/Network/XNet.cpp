#include "stdafx.h"
#include "XNet.h"
#include "XPeer.h"

#include "Util\LibaryStartup.h"

#include <Time\DateStamp.h>
#include <ITimer.h>

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

}

void XNet::registerCmds(void)
{

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

	PlatLib::deRef();

}

void XNet::release(void)
{

}


IPeer* XNet::createPeer(void)
{
	return X_NEW(XPeer, arena_, "Peer")(arena_);
}

void XNet::deletePeer(IPeer* pPeer)
{
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

	uint64_t val = digest.data[0] | digest.data[1];

	return NetGUID(val);
}

X_NAMESPACE_END


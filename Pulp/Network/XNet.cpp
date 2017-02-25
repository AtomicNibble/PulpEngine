#include "stdafx.h"
#include "XNet.h"
#include "XPeer.h"


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

	return true;
}

void XNet::shutDown(void)
{

}

void XNet::release(void)
{

}


IPeer* XNet::createPeer(void)
{
	return X_NEW(XPeer, arena_, "Peer")();
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


X_NAMESPACE_END


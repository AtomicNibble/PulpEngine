#pragma once


X_NAMESPACE_BEGIN(net)

class XNet : public INet
{
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

	ISystemAdd* createSysAddress(const char* pAddressStr) X_FINAL;
	ISystemAdd* createSysAddress(const char* pAddressStr, uint16_t port) X_FINAL;
	// ~INet

private:
	core::MemoryArenaBase* arena_;
};


X_NAMESPACE_END


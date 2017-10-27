#pragma once


X_NAMESPACE_BEGIN(video)


class XVideoSys : public IVideoSys
{
public:
	XVideoSys(core::MemoryArenaBase* arena);
	~XVideoSys() X_FINAL = default;

	void registerVars(void) X_FINAL;
	void registerCmds(void) X_FINAL;

	bool init(void) X_FINAL;
	void shutDown(void) X_FINAL;
	void release(void) X_FINAL;

private:

};


X_NAMESPACE_END
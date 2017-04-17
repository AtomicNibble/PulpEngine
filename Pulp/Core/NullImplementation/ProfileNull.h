#pragma once



X_NAMESPACE_BEGIN(core)


class ProfileNull : public IProfileSys
{
	void registerVars(void) X_FINAL;
	void registerCmds(void) X_FINAL;

	bool init(ICore* pCore) X_FINAL;
	void shutDown(void) X_FINAL;

	void AddProfileData(XProfileData* pData) X_FINAL;

	void OnFrameBegin(void) X_FINAL;
	void OnFrameEnd(void) X_FINAL;

	void Render(void) X_FINAL;
};


X_NAMESPACE_END



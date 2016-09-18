#pragma once


class asIScriptEngine;
class asIScriptContext;
struct asSMessageInfo;

X_NAMESPACE_BEGIN(assman)


class AssetScript
{
public:
	AssetScript();
	~AssetScript();

	bool init(void);
	void shutdown(void);


	bool processScript(const char* pFileName);

private:
	static void messageCallback(const asSMessageInfo *msg, void *param);
	static void printExceptionInfo(asIScriptContext *ctx);

private:
	asIScriptEngine* pEngine_;

};



X_NAMESPACE_END
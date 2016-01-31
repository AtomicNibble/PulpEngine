#pragma once


#include <IConverterModule.h>
#include <String\CmdArgs.h>

X_NAMESPACE_DECLARE(anim,
struct 	IAnimLib
)

X_NAMESPACE_DECLARE(model,
struct IModelLib
)

X_NAMESPACE_BEGIN(converter)

X_DECLARE_ENUM(AssetType)(ANIM, MODEL);

class Converter
{
	typedef core::traits::Function<void *(ICore *pSystem, const char *moduleName)> ModuleLinkfunc;
	typedef core::CmdArgs<4096, wchar_t> ConvertArgs;

public:
	Converter();
	~Converter();

	void PrintBanner(void);

	bool Convert(AssetType::Enum type, ConvertArgs& args);


private:
	bool EnsureLibLoaded(AssetType::Enum type);

	bool LoadAnimLib(void);
	bool LoadModelLib(void);

	void* LoadDLL(const char* dllName);
	bool IntializeConverterModule(const char* dllName, const char* moduleClassName);

private:
	ConverterLibs libs_;
};


X_NAMESPACE_END
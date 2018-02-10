#pragma once

X_NAMESPACE_BEGIN(engine)

namespace fx
{

	class FxLib : public IFxLib
	{
	public:
		FxLib();
		~FxLib() X_OVERRIDE;

		virtual const char* getOutExtension(void) const X_OVERRIDE;

		virtual bool Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath) X_OVERRIDE;
	};


} // namespace fx

X_NAMESPACE_END
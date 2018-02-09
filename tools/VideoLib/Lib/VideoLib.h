#pragma once

X_NAMESPACE_BEGIN(video)


	class VideoLib : public IVideoLib
	{
	public:
		VideoLib();
		~VideoLib() X_OVERRIDE;

		virtual const char* getOutExtension(void) const X_OVERRIDE;

		virtual bool Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath) X_OVERRIDE;
	};


X_NAMESPACE_END
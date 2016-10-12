#pragma once

#include <QObject>


#include <../ConverterLib/ConverterLib.h>
// #include <../AssetDB/AssetDB.h>

#include <Threading\ThreadQue.h>

X_NAMESPACE_DECLARE(assetDb,
	class AssetDB;
);

X_NAMESPACE_BEGIN(assman)


class ConverterHost : public QThread
{
	Q_OBJECT

private:
	enum class ConversionType {
		SINGLE,
		MOD,
		MOD_TYPE,
		ALL,

		EXIT
	};

	struct ConversionJob
	{
		ConversionType conType;
		core::string name;
		assetDb::AssetType::Enum type;
		int32_t modId;
	};

public:
	ConverterHost(assetDb::AssetDB& db, core::MemoryArenaBase* arena);
	~ConverterHost();

	void init();
	void shutdown();

	void convertAsset(const core::string& name, assetDb::AssetType::Enum type);
	void convertMod(int32_t modId);
	void convertMod(int32_t modId, assetDb::AssetType::Enum type);

private:
	void postQuitJob(void);

protected:
	void run();

private:
	core::ThreadQueBlocking<ConversionJob, core::CriticalSection> que_;

	converter::Converter con_;
};

X_NAMESPACE_END
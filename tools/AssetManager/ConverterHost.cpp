#include "ConverterHost.h"

#include <Threading\JobSystem2.h>


X_NAMESPACE_BEGIN(assman)


ConverterHost::ConverterHost(assetDb::AssetDB& db, core::MemoryArenaBase* arena) :
	que_(arena, 16),
	con_(db, arena)
{

}

ConverterHost::~ConverterHost()
{

}

void ConverterHost::init()
{
	start();
}

void ConverterHost::shutdown()
{
	postQuitJob();
	wait();
}

void ConverterHost::convertAsset(const core::string& name, assetDb::AssetType::Enum type)
{
	ConversionJob job;
	job.conType = ConversionType::SINGLE;
	job.name = name;
	job.type = type;

	que_.push(job);
}

void ConverterHost::convertMod(int32_t modId)
{
	ConversionJob job;
	job.conType = ConversionType::MOD;
	job.modId = modId;

	que_.push(job);
}

void ConverterHost::convertMod(int32_t modId, assetDb::AssetType::Enum type)
{
	ConversionJob job;
	job.conType = ConversionType::MOD_TYPE;
	job.modId = modId;
	job.type = type;

	que_.push(job);
}


void ConverterHost::postQuitJob()
{
	ConversionJob job;
	job.conType = ConversionType::EXIT;

	que_.push(job);
}

void ConverterHost::run()
{
	gEnv->pJobSys->CreateQueForCurrentThread();

	while (1)
	{
		ConversionJob job;
		que_.pop(job);

		if (job.conType == ConversionType::SINGLE)
		{
			con_.Convert(job.type, job.name);
		}
		else if (job.conType == ConversionType::MOD)
		{
			con_.Convert(job.modId);
		}
		else if (job.conType == ConversionType::MOD_TYPE)
		{
			con_.Convert(job.modId, job.type);
		}
		else if (job.conType == ConversionType::ALL)
		{
			con_.ConvertAll();
		}
		else if (job.conType == ConversionType::EXIT)
		{
			break;
		}

		int goat = 0;
		goat = 2;
	}
}

X_NAMESPACE_END
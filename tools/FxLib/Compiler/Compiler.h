#pragma once


#include <Containers\ByteStream.h>

X_NAMESPACE_DECLARE(core,
	struct XFile;
);

X_NAMESPACE_BEGIN(engine)

namespace fx
{

	class EffectCompiler
	{
		typedef core::Array<uint8_t> DataVec;

		typedef core::Array<Stage> StageArr;

	public:
		EffectCompiler(core::MemoryArenaBase* arena);
		~EffectCompiler();

		bool writeToFile(core::XFile* pFile) const;

	private:
		core::MemoryArenaBase* arena_;
		StageArr stages_;
	};

} // namespace fx


X_NAMESPACE_END
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

	public:
		EffectCompiler(core::MemoryArenaBase* arena);
		~EffectCompiler();

	private:
		core::MemoryArenaBase* arena_;
	};

} // namespace fx


X_NAMESPACE_END
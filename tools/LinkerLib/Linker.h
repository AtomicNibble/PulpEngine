#pragma once

X_NAMESPACE_BEGIN(linker)

class Linker
{

public:
	LINKERLIB_EXPORT Linker(core::MemoryArenaBase* scratchArea);
	LINKERLIB_EXPORT ~Linker();

	LINKERLIB_EXPORT void PrintBanner(void);
	LINKERLIB_EXPORT bool Init(void);

private:

};


X_NAMESPACE_END

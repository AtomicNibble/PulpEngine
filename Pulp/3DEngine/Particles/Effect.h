#pragma once


X_NAMESPACE_BEGIN(engine)


class Effect : public core::AssetBase
{
	X_NO_COPY(Effect);
	X_NO_ASSIGN(Effect);

public:
	Effect(core::string& name, core::MemoryArenaBase* arena);
	~Effect();


};


X_NAMESPACE_END
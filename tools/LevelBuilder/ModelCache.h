#pragma once


X_NAMESPACE_BEGIN(lvl)

class ModelCache
{
	typedef core::HashMap<core::string, AABB> ModelMap;

public:
	ModelCache(core::MemoryArenaBase* arena);
	~ModelCache();

	bool getModelAABB(const core::string& name, AABB& bounds);
	bool loadDefaultModel(void);

private:


private:
	core::MemoryArenaBase* arena_;
	ModelMap models_;

	AABB defaultModelBounds_;
};


X_NAMESPACE_END

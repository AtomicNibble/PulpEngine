#pragma once

X_NAMESPACE_BEGIN(lvl)


class Compiler
{
public:
	Compiler(core::MemoryArenaBase* arena, physics::IPhysicsCooking* pPhysCooking);
	~Compiler();

	bool compileLevel(core::Path<char>& path);

private:
	core::MemoryArenaBase* arena_;
	physics::IPhysicsCooking* pPhysCooking_;
};


X_NAMESPACE_END
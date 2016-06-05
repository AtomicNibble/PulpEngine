#pragma once



X_NAMESPACE_DECLARE(core,
	struct ICVar;
)


X_NAMESPACE_BEGIN(physics)


class PhysXVars
{
public:
	PhysXVars();
	~PhysXVars() = default;

	void RegisterVars(void);

	uint32_t ScratchBufferSize(void) const;


private:
	core::ICVar* pVarScratchBufSize_;

	int32_t scratchBufferDefaultSize_;
};




X_NAMESPACE_END
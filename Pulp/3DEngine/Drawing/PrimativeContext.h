#pragma once

#include "IPrimativeContext.h"

X_NAMESPACE_BEGIN(engine)

class PrimativeContext : public IPrimativeContext
{
public:

public:
	PrimativeContext();
	~PrimativeContext() X_OVERRIDE;


private:
	Vertex_P3F_T2F_C4B* addPrimative(uint32_t num, PrimitiveType::Enum type, texture::TexID texture_id) X_FINAL;
	Vertex_P3F_T2F_C4B* addPrimative(uint32_t num, PrimitiveType::Enum type) X_FINAL;

};


X_NAMESPACE_END
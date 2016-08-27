#include "stdafx.h"
#include "PrimativeContext.h"

#include "Math\VertexFormats.h"

X_NAMESPACE_BEGIN(engine)

PrimativeContext::PrimativeContext()
{

}

PrimativeContext::~PrimativeContext() 
{

}



Vertex_P3F_T2F_C4B* PrimativeContext::addPrimative(uint32_t num, PrimitiveType::Enum type, texture::TexID texture_id)
{
	X_ASSERT_NOT_IMPLEMENTED();
	X_UNUSED(num);
	X_UNUSED(type);
	X_UNUSED(texture_id);
	return nullptr;
}

Vertex_P3F_T2F_C4B* PrimativeContext::addPrimative(uint32_t num, PrimitiveType::Enum type)
{
	X_ASSERT_NOT_IMPLEMENTED();
	X_UNUSED(num);
	X_UNUSED(type);
	return nullptr;
}

X_NAMESPACE_END
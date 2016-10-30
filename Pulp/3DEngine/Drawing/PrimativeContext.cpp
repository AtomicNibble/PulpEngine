#include "stdafx.h"
#include "PrimativeContext.h"

#include "Math\VertexFormats.h"

X_NAMESPACE_BEGIN(engine)

PrimativeContext::PrimativeContext(core::MemoryArenaBase* arena) :
	pushBufferArr_(arena),
	vertexArr_(arena)
{
	
	pushBufferArr_.reserve(64);
	vertexArr_.reserve(256);
}

PrimativeContext::~PrimativeContext() 
{

}

void PrimativeContext::reset(void)
{
	pushBufferArr_.clear();
	vertexArr_.clear();
}

bool PrimativeContext::isEmpty(void) const
{
	return pushBufferArr_.isEmpty();
}

void PrimativeContext::getSortedBuffer(SortedPushBufferArr& sortedPushBuffer) const
{
	sortedPushBuffer.reserve(pushBufferArr_.size());
	sortedPushBuffer.resize(0);

	for (const auto& ap : pushBufferArr_) {
		sortedPushBuffer.push_back(&ap);
	}

	std::sort(sortedPushBuffer.begin(), sortedPushBuffer.end(),
		[](const PushBufferEntry* lhs, const PushBufferEntry* rhs) {
			return lhs->flags < rhs->flags;
		}
	);
}

const PrimativeContext::PushBufferArr& PrimativeContext::getUnsortedBuffer(void) const
{
	return pushBufferArr_;
}

void PrimativeContext::drawTextQueued(Vec3f pos, const render::XDrawTextInfo& ti, const char* pText)
{
	X_UNUSED(pos);
	X_UNUSED(ti);
	X_UNUSED(pText);

}

PrimativeContext::PrimVertex* PrimativeContext::addPrimative(uint32_t numVertices, PrimitiveType::Enum primType, texture::TexID textureId)
{
	PrimRenderFlags flags(primType, textureId);

	// if the last entry was the same type
	// just merge the verts in.
	if (pushBufferArr_.isNotEmpty() && pushBufferArr_.back().flags == flags &&
		(PrimitiveType::POINTLIST == primType || PrimitiveType::LINELIST == primType || PrimitiveType::TRIANGLELIST == primType))
	{
		auto& lastEntry = pushBufferArr_.back();
		lastEntry.numVertices += numVertices;
	}
	else
	{
		pushBufferArr_.emplace_back(numVertices,
			safe_static_cast<uint32_t, size_t>(vertexArr_.size()),
			flags
		);
	}

#if X_DEBUG
	// if flushing is never performed this will just keep growing.
	// so lets check we not going over some fairly large count.
	// if we exceede this and it's ok increase the check.
	if (pushBufferArr_.size() > 4096)
	{
		X_WARNING_EVERY_N(50, "PrimContext", "PrimContext has %i entryies did you forget to flush?", pushBufferArr_.size());
	}
#endif // !X_DEBUG

	// resize and return.
	auto& vertexArr = vertexArr_;
	const auto oldVBSize = vertexArr.size();
	vertexArr.resize(oldVBSize + numVertices);

	return &vertexArr[oldVBSize];
}

PrimativeContext::PrimVertex* PrimativeContext::addPrimative(uint32_t numVertices, PrimitiveType::Enum primType)
{
	return addPrimative(numVertices, primType, 0);
}

X_NAMESPACE_END
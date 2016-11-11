#include "stdafx.h"
#include "PrimativeContext.h"

#include "Math\VertexFormats.h"

#include <IRender.h>
#include <IShader.h>

X_NAMESPACE_BEGIN(engine)

PrimativeContext::PrimativeContext(core::MemoryArenaBase* arena) :
	pushBufferArr_(arena),
	vertexArr_(arena)
{
	pushBufferArr_.reserve(64);
	vertexArr_.getAllocator().setBaseAlignment(16); // for simd.
	vertexArr_.reserve(256);

	passHandle_ = render::INVALID_STATE_HANLDE;
	for (auto& h : stateCache_)
	{
		h = render::INVALID_STATE_HANLDE;
	}
}

PrimativeContext::~PrimativeContext() 
{

}

bool PrimativeContext::createStates(render::IRender* pRender)
{
	render::StateDesc desc;
	desc.blend.srcBlendColor = render::BlendType::SRC_ALPHA;
	desc.blend.srcBlendAlpha = render::BlendType::SRC_ALPHA;
	desc.blend.dstBlendColor = render::BlendType::INV_SRC_ALPHA;
	desc.blend.dstBlendAlpha = render::BlendType::INV_SRC_ALPHA;
	desc.blendOp = render::BlendOp::OP_ADD;
	desc.cullType = render::CullType::TWO_SIDED;
	desc.depthFunc = render::DepthFunc::ALWAYS;
	desc.stateFlags.Clear();
	desc.stateFlags.Set(render::StateFlag::BLEND);
	desc.stateFlags.Set(render::StateFlag::NO_DEPTH_TEST);
	desc.vertexFmt = render::shader::VertexFormat::P3F_T2F_C4B;


	const auto* pShader = pRender->getShader("AuxGeom");
	const auto* pTech = pShader->getTech("AuxGeometry");

	auto renderTarget = pRender->getCurBackBuffer();

	render::RenderTargetFmtsArr rtfs;
	rtfs.append(renderTarget->getFmt());

	passHandle_ = pRender->createPassState(rtfs);

	for (size_t i = 0; i < PrimitiveType::ENUM_COUNT; i++)
	{
		const auto primType = static_cast<PrimitiveType::Enum>(i);

		desc.topo = primType;
		stateCache_[primType] = pRender->createState(passHandle_, pTech, desc, nullptr, 0);

		if (stateCache_[primType] == render::INVALID_STATE_HANLDE) {
			X_ERROR("PrimContext", "Failed to create state for primType: \"%s\"", PrimitiveType::ToString(primType));
			return false;
		}
	}

	const auto* pTextShader = pRender->getShader("Font");
	const auto* pTextTech = pTextShader->getTech("Font");

	// this root sig needs to have one text handler.
	// how to handle the problem of diffrent shaders having diffrent texture slot requirements.
	// which we need to know about when creating a root sig that works with that shader.
	// so maybe we should just work it out from the shader. :D

	desc.topo = PrimitiveType::TRIANGLELIST;
	textDrawState_ = pRender->createState(passHandle_, pTextTech, desc, nullptr, 0);


	return true;
}

bool PrimativeContext::freeStates(render::IRender* pRender)
{
	for (auto& state : stateCache_)
	{
		if (state != render::INVALID_STATE_HANLDE) {
			pRender->destoryState(state);
		}
	}

	pRender->destoryPassState(passHandle_);
	return true;
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

const PrimativeContext::VertexArr& PrimativeContext::getVerts(void) const
{
	return vertexArr_;
}



void PrimativeContext::drawText(const Vec3f& pos, const font::TextDrawContext& ctx, const char* pBegin, const char* pEnd)
{
	// so we need todo the cpu side font rendering task here.
	// so we need the font to create all the verts for us.
	// and we need a texture id to draw with.
	X_ASSERT_NOT_NULL(ctx.pFont);

	// we just send outself to the fonts render function that way the font can add what primatives it wishes.
	ctx.pFont->DrawString(this, textDrawState_, pos, ctx, pBegin, pEnd);
}

PrimativeContext::PrimVertex* PrimativeContext::addPrimative(uint32_t numVertices, PrimitiveType::Enum primType,
	texture::TexID textureId, render::StateHandle stateHandle)
{
	PrimRenderFlags flags(primType, textureId);

	// if the last entry was the same type
	// just merge the verts in.
	if (pushBufferArr_.isNotEmpty() && pushBufferArr_.back().flags == flags && pushBufferArr_.back().stateHandle == stateHandle &&
		(PrimitiveType::POINTLIST == primType || PrimitiveType::LINELIST == primType || PrimitiveType::TRIANGLELIST == primType))
	{
		auto& lastEntry = pushBufferArr_.back();
		lastEntry.numVertices += numVertices;
	}
	else
	{
		pushBufferArr_.emplace_back(numVertices,
			safe_static_cast<uint32_t, size_t>(vertexArr_.size()),
			flags,
			stateHandle
		);
	}

	// resize and return.
	const auto oldVBSize = vertexArr_.size();
	vertexArr_.resize(oldVBSize + numVertices);

	return &vertexArr_[oldVBSize];
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
			flags,
			stateCache_[primType]
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
	const auto oldVBSize = vertexArr_.size();
	vertexArr_.resize(oldVBSize + numVertices);

	return &vertexArr_[oldVBSize];
}

PrimativeContext::PrimVertex* PrimativeContext::addPrimative(uint32_t numVertices, PrimitiveType::Enum primType)
{
	return addPrimative(numVertices, primType, 0);
}

X_NAMESPACE_END
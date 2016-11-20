#include "stdafx.h"
#include "PrimativeContext.h"

#include "Math\VertexFormats.h"

#include <IRender.h>
#include <IShader.h>
#include <CBuffer.h>

#include "CmdBucket.h"

X_NAMESPACE_BEGIN(engine)


PrimativeContext::VertexPage::VertexPage(core::MemoryArenaBase* arena) :
	vertexBufHandle(render::INVALID_BUF_HANLDE),
	verts(arena)
{

	verts.getAllocator().setBaseAlignment(16); // for simd.
	verts.setGranularity(1024 * 4);
}


void PrimativeContext::VertexPage::reset(void)
{
	verts.clear();
}

void PrimativeContext::VertexPage::createVB(render::IRender* pRender)
{
	X_ASSERT(vertexBufHandle == render::INVALID_BUF_HANLDE, "Vertex buffer already valid")();

	vertexBufHandle = pRender->createVertexBuffer(
		sizeof(IPrimativeContext::PrimVertex),
		NUMVERTS_PER_PAGE,
		render::BufUsage::DYNAMIC,
		render::CpuAccess::WRITE
	);
}

void PrimativeContext::VertexPage::destoryVB(render::IRender* pRender)
{
	// allow to be called if invalid
	if (isVbValid()) {
		pRender->destoryVertexBuffer(vertexBufHandle);
	}
}

bool PrimativeContext::VertexPage::isVbValid(void) const
{
	return vertexBufHandle != render::INVALID_BUF_HANLDE;
}


const uint32_t PrimativeContext::VertexPage::getVertBufBytes(void) const
{
	uint32_t numVerts = safe_static_cast<uint32_t>(verts.size());


	X_ASSERT(numVerts > NUMVERTS_PER_PAGE, "Vert page exceeded it's limits")(numVerts, NUMVERTS_PER_PAGE);

	// we need to give the render system a 16byte aligned buffer that is a multiple of 16 bytes.
	// i think i might support not requiring the buffer size to be a multiple of 16 as that means we always need padding if vert not multiple of 16.
	return core::bitUtil::RoundUpToMultiple<uint32_t>(numVerts * sizeof(PrimVertex), 16u);
}

const uint32_t PrimativeContext::VertexPage::freeSpace(void) const
{
	return NUMVERTS_PER_PAGE - safe_static_cast<uint32_t>(verts.size());
}


// ---------------------------------------------------------------------------

PrimativeContext::PrimativeContext(core::MemoryArenaBase* arena) :
	pRender_(nullptr),
	pushBufferArr_(arena),
	vertexPages_(arena, MAX_PAGES, arena),
	currentPage_(0),
	pAuxShader_(nullptr),
	pTextShader_(nullptr)
{
	pushBufferArr_.reserve(64);
	pushBufferArr_.setGranularity(512);

	X_ASSERT(vertexPages_.isNotEmpty(), "Must have atleast one vertex page")();
	// for the fist page we reverse something small
	// before setting the large granularity.
	// that way we grow fast but if not rendering much we stay small.

	passHandle_ = render::INVALID_STATE_HANLDE;
	textDrawState_ = render::INVALID_STATE_HANLDE;
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
	pRender_ = pRender;

	render::StateDesc desc;
	desc.blend.srcBlendColor = render::BlendType::SRC_ALPHA;
	desc.blend.srcBlendAlpha = render::BlendType::SRC_ALPHA;
	desc.blend.dstBlendColor = render::BlendType::INV_SRC_ALPHA;
	desc.blend.dstBlendAlpha = render::BlendType::INV_SRC_ALPHA;
	desc.blendOp = render::BlendOp::OP_ADD;
	desc.cullType = render::CullType::NONE;
	desc.depthFunc = render::DepthFunc::ALWAYS;
	desc.stateFlags.Clear();
	desc.stateFlags.Set(render::StateFlag::BLEND);
	desc.stateFlags.Set(render::StateFlag::NO_DEPTH_TEST);
	desc.vertexFmt = render::shader::VertexFormat::P3F_T2F_C4B;


	pAuxShader_ = pRender->getShader("Prim");
	auto* pTech = pAuxShader_->getTech("Prim");
	pTech->tryCompile(true);
	auto* pPerm = pTech->getPermatation(desc.vertexFmt);

	{
		// can get a list of all the cbuffers needed for this permatation.
		const auto& cbufs = pPerm->getCbufferLinks();
		for (auto& cb : cbufs)
		{
			// for the prim contex we only support shaders that make use of automated params.
			// this way you can edit it as much as you want without needing to edit engine code.
			// as the cbuffers can be populated automatically.
			if (cb.pCBufer->requireManualUpdate())
			{
				X_ERROR("PrimContext", "Prim technique has a cbuffer \"%s\" that requires manual update", cb.pCBufer->getName().c_str());
				return false;
			}
		}
	}

	auto renderTarget = pRender->getCurBackBuffer();

	render::RenderTargetFmtsArr rtfs;
	rtfs.append(renderTarget->getFmt());

	passHandle_ = pRender->createPassState(rtfs);

	for (size_t i = 0; i < PrimitiveType::ENUM_COUNT; i++)
	{
		const auto primType = static_cast<PrimitiveType::Enum>(i);

		desc.topo = primType;
		stateCache_[primType] = pRender->createState(passHandle_, pPerm, desc, nullptr, 0);

		if (stateCache_[primType] == render::INVALID_STATE_HANLDE) {
			X_ERROR("PrimContext", "Failed to create state for primType: \"%s\"", PrimitiveType::ToString(primType));
			return false;
		}
	}

	pTextShader_ = pRender->getShader("Font");
	auto* pTextTech = pTextShader_->getTech("Font");
	pTextTech->tryCompile(true);

	pPerm = pTextTech->getPermatation(desc.vertexFmt);

	{
		const auto& cbufs = pPerm->getCbufferLinks();
		for (auto& cb : cbufs)
		{
			if (cb.pCBufer->requireManualUpdate())
			{
				X_ERROR("PrimContext", "Font technique has a cbuffer \"%s\" that requires manual update", cb.pCBufer->getName().c_str());
				return false;
			}
		}
	}

	// this root sig needs to have one text handler.
	// how to handle the problem of diffrent shaders having diffrent texture slot requirements.
	// which we need to know about when creating a root sig that works with that shader.
	// so maybe we should just work it out from the shader. :D

	desc.topo = PrimitiveType::TRIANGLELIST;
	textDrawState_ = pRender->createState(passHandle_, pPerm, desc, nullptr, 0);



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

	if (textDrawState_ != render::INVALID_STATE_HANLDE) {
		pRender->destoryState(textDrawState_);
	}

	if (passHandle_ != render::INVALID_STATE_HANLDE) {
		pRender->destoryPassState(passHandle_);
	}

#if X_DEBUG
	passHandle_ = render::INVALID_STATE_HANLDE;
	textDrawState_ = render::INVALID_STATE_HANLDE;
	for (auto& h : stateCache_)
	{
		h = render::INVALID_STATE_HANLDE;
	}
#endif // X_DEBUG

	if (pAuxShader_) {
		pRender->releaseShader(pAuxShader_);
	}
	if (pTextShader_) {
		pRender->releaseShader(pTextShader_);
	}

	for (auto& vp : vertexPages_) {
		vp.destoryVB(pRender);
	}

	// clear the member render pointer, nothing should be trying to use it after a state clear :|
	pRender_ = nullptr;
	return true;
}

void PrimativeContext::appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const
{
	// any pages that have verts we update the gpu buffers.
	for (const auto& vp : vertexPages_)
	{
		if (vp.verts.isEmpty()) {
			break;
		}

		auto* pUpdateVb = bucket.addCommand<render::Commands::CopyVertexBufferData>(0, 0);
		pUpdateVb->pData = vp.verts.data();
		pUpdateVb->size = vp.getVertBufBytes();
		pUpdateVb->vertexBuffer = vp.vertexBufHandle;
	}

#if X_DEBUG
	bool expectNull = false;
	for (size_t i=0; i<vertexPages_.size(); i++)
	{
		if (expectNull) {
			X_ASSERT(vertexPages_[i].verts.isEmpty(), "Avertex page had data after a page that was empty")();
		}
		else
		{
			if (vertexPages_[i].verts.isEmpty()) {
				expectNull = true;
			}
		}
	}
#endif // X_DEBUG
}

void PrimativeContext::reset(void)
{
	pushBufferArr_.clear();

	currentPage_ = 0;

	for (auto& vp : vertexPages_) {
		vp.reset();
	}
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

PrimativeContext::VertexPageHandlesArr PrimativeContext::getVertBufHandles(void) const
{
	VertexPageHandlesArr handles;

	handles.fill(render::INVALID_BUF_HANLDE);

	for (size_t i = 0; i < vertexPages_.size(); i++) {
		handles[i] = vertexPages_[i].vertexBufHandle;
	}

	return handles;
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

void PrimativeContext::drawText(const Vec3f& pos, const font::TextDrawContext& ctx, const wchar_t* pBegin, const wchar_t* pEnd)
{
	X_ASSERT_NOT_NULL(ctx.pFont);

	ctx.pFont->DrawString(this, textDrawState_, pos, ctx, pBegin, pEnd);
}

PrimativeContext::VertexPage& PrimativeContext::getPage(size_t requiredVerts)
{
	if (vertexPages_[currentPage_].freeSpace() > requiredVerts) {
		return vertexPages_[currentPage_];
	}

	// switch page.
	++currentPage_;

	auto& newPage = vertexPages_[currentPage_];
	if(!newPage.isVbValid()) {
		// virgin page, need a good slapping..
		// renderSys support creating vertexBuffers in parralel from multiple threads so this is fine.
		newPage.createVB(pRender_);
	}

	return newPage;
}

PrimativeContext::PrimVertex* PrimativeContext::addPrimative(uint32_t numVertices, PrimitiveType::Enum primType,
	texture::TexID textureId, render::StateHandle stateHandle)
{
	PrimRenderFlags flags(primType, textureId);

	auto& curPage = getPage(numVertices);
	auto& vertexArr = curPage.verts;

	// if the last entry was the same type
	// just merge the verts in.
	if (pushBufferArr_.isNotEmpty() && pushBufferArr_.back().flags == flags && pushBufferArr_.back().stateHandle == stateHandle &&
		(PrimitiveType::POINTLIST == primType || PrimitiveType::LINELIST == primType || PrimitiveType::TRIANGLELIST == primType))
	{
		auto& lastEntry = pushBufferArr_.back();
		lastEntry.numVertices += safe_static_cast<uint16_t>(numVertices);
	}
	else
	{
		pushBufferArr_.emplace_back(
			safe_static_cast<uint16_t>(numVertices),
			safe_static_cast<uint16_t>(vertexArr.size()),
			currentPage_,
			flags,
			stateHandle
		);
	}

	// resize and return.
	const auto oldVBSize = vertexArr.size();
	vertexArr.resize(oldVBSize + numVertices);

	return &vertexArr[oldVBSize];
}

PrimativeContext::PrimVertex* PrimativeContext::addPrimative(uint32_t numVertices, PrimitiveType::Enum primType, texture::TexID textureId)
{
	return addPrimative(numVertices, primType, textureId, stateCache_[primType]);
}

PrimativeContext::PrimVertex* PrimativeContext::addPrimative(uint32_t numVertices, PrimitiveType::Enum primType)
{
	return addPrimative(numVertices, primType, 0);
}

X_NAMESPACE_END
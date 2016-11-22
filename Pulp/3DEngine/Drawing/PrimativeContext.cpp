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


// ---------------------------------------------------------------------------

PrimativeContext::PrimativeContext(core::MemoryArenaBase* arena) :
	pRender_(nullptr),
	pushBufferArr_(arena),
	vertexPages_(arena, MAX_PAGES, arena),
	currentPage_(-1),
	pAuxShader_(nullptr)
{
	pushBufferArr_.reserve(64);
	pushBufferArr_.setGranularity(512);

	X_ASSERT(vertexPages_.isNotEmpty(), "Must have atleast one vertex page")();
	// for the fist page we reverse something small
	// before setting the large granularity.
	// that way we grow fast but if not rendering much we stay small.

	passHandle_ = render::INVALID_STATE_HANLDE;
	for (auto& h : stateCache_)
	{
		h = render::INVALID_STATE_HANLDE;
	}

	core::zero_object(primMaterials_);
}


PrimativeContext::~PrimativeContext() 
{

}

bool PrimativeContext::createStates(render::IRender* pRender, IMaterialManager* pMatMan)
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
		core::StackString<64, char> name("$prim_");
		name.append(PrimitiveType::ToString(primType));

		XMaterial* pMat = pMatMan->createMaterial(name.c_str());

		desc.topo = primType;
		stateCache_[primType] = pRender->createState(passHandle_, pPerm, desc, nullptr, 0);

		if (stateCache_[primType] == render::INVALID_STATE_HANLDE) {
			X_ERROR("PrimContext", "Failed to create state for primType: \"%s\"", PrimitiveType::ToString(primType));
			return false;
		}

		pMat->setStateDesc(desc);
		pMat->setStateHandle(stateCache_[primType]);
		pMat->setCat(MaterialCat::CODE);
		
		// how to pick shader and tech.
		// and also permatation.
		// plus creaete a variable state that can store the shit.
		// well we are only creating materials for stuff we know is simple primatives with no textures.
		// so we know the variable state needs no textures and if we select the tech's 


		// MeOwWwWWW !!!
		primMaterials_[primType] = pMat;
	}


	// what we essentialy need is a variable state that has the const buffer handles in.
	// we create handles for the const buffers and then each frame we ask the CB manager to update us.
	// that way we only attemp the update CB's when we plan to use them.
	// The other way i was thinking was registering const buffers and the manager would update them each frame if changed.
	// but that has the problem of updating const buffers every frame that may never be used.
	// then we also need variable states for when we want to render with textures.
	// we we render with a texture we need a diffrent shader and also the texture id and wrap mode.
	// which is all info store in a material.
	// so i want to move to rendering with materials.
	// so when the GUI has a quad with a material it just sets the material.
	// and we get the variable state from the material which will contain the texture handles needed and the correct PSO
	// for use to render the textures.
	// it may also contain additional const buffers if the material supports scrolling uv's
	// so by supporting materials this prim context then automatically supports rendering quads that have scrolling uv's
	// without the prim context having todo anything but render what it's told.
	// so my fat nigerian goat, we need to make a material for drawing primative shit.




	// i think i'll move this into the font's that way they can define wierd pipeline states for each font if they want.
#if 0
	{
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

	}
#endif
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

	if (passHandle_ != render::INVALID_STATE_HANLDE) {
		pRender->destoryPassState(passHandle_);
	}

#if X_DEBUG
	passHandle_ = render::INVALID_STATE_HANLDE;
	for (auto& h : stateCache_)
	{
		h = render::INVALID_STATE_HANLDE;
	}
#endif // X_DEBUG

	if (pAuxShader_) {
		pRender->releaseShader(pAuxShader_);
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

		X_ASSERT(vp.vertexBufHandle != render::INVALID_BUF_HANLDE, "Vertex buffer handle should be valid")();

		auto* pUpdateVb = bucket.addCommand<render::Commands::CopyVertexBufferData>(0, 0);
		pUpdateVb->pData = vp.verts.data();
		pUpdateVb->size = vp.getVertBufBytes();
		pUpdateVb->vertexBuffer = vp.vertexBufHandle;
	}

#if X_DEBUG // check that a page was not 'skipped' or not reset correct.
	bool expectNull = false;
	for (size_t i=0; i<vertexPages_.size(); i++)
	{
		if (expectNull) {
			X_ASSERT(vertexPages_[i].verts.isEmpty(), "A vertex page had data after a page that was empty")();
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

	currentPage_ = -1;

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

	// what to sort by :Z ?
	X_ASSERT_NOT_IMPLEMENTED();
//	std::sort(sortedPushBuffer.begin(), sortedPushBuffer.end(),
//		[](const PushBufferEntry* lhs, const PushBufferEntry* rhs) {
//			return lhs->flags < rhs->flags;
//		}
//	);
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
	ctx.pFont->DrawString(this, pos, ctx, pBegin, pEnd);
}

void PrimativeContext::drawText(const Vec3f& pos, const font::TextDrawContext& ctx, const wchar_t* pBegin, const wchar_t* pEnd)
{
	X_ASSERT_NOT_NULL(ctx.pFont);

	ctx.pFont->DrawString(this, pos, ctx, pBegin, pEnd);
}

PrimativeContext::VertexPage& PrimativeContext::getPage(size_t requiredVerts)
{
	if (currentPage_ >= 0 && vertexPages_[currentPage_].freeSpace() > requiredVerts) {
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
	XMaterial* pMaterial)
{
	auto& curPage = getPage(numVertices);
	auto& vertexArr = curPage.verts;

	// if the last entry was the same type
	// just merge the verts in.
	if (pushBufferArr_.isNotEmpty() && pushBufferArr_.back().pMaterial == pMaterial &&
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
			pMaterial
		);
	}

	// resize and return.
	const auto oldVBSize = vertexArr.size();
	vertexArr.resize(oldVBSize + numVertices);

	return &vertexArr[oldVBSize];
}

PrimativeContext::PrimVertex* PrimativeContext::addPrimative(uint32_t numVertices, PrimitiveType::Enum primType)
{
	return addPrimative(numVertices, primType, primMaterials_[primType]);
}

X_NAMESPACE_END
#include "stdafx.h"
#include "PrimativeContext.h"

#include "Math\VertexFormats.h"

#include <IRender.h>
#include <IShader.h>
#include <CBuffer.h>
#include <IAssetDb.h>

#include "CmdBucket.h"
#include "VariableStateManager.h"

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

PrimativeContext::PrimativeContext(Mode mode, core::MemoryArenaBase* arena) :
	pRender_(nullptr),
	pushBufferArr_(arena),
	vertexPages_(arena, MAX_PAGES, arena),
	currentPage_(-1),
	mode_(mode),
	objects_{ arena, arena, arena } 
{
	pushBufferArr_.reserve(64);
	pushBufferArr_.setGranularity(512);

	for (auto& objectArr : objects_)
	{
		objectArr.setGranularity(256);
	}

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

bool PrimativeContext::createStates(render::IRender* pRender)
{
	pRender_ = pRender;

	for (size_t i = 0; i < PrimitiveType::ENUM_COUNT; i++)
	{
		const auto primType = static_cast<PrimitiveType::Enum>(i);

		core::StackString<assetDb::ASSET_NAME_MAX_LENGTH, char> matName;
		matName.appendFmt("code%c$%s", assetDb::ASSET_NAME_SLASH, PrimitiveType::ToString(primType));
		matName.toLower();

		// we load the material for the primative type :D
		// so the state and shaders used is fully data driven MMMMM.
		Material* pMat = pMaterialManager_->loadMaterial(matName.c_str());

		// we still assign even tho may be default so it get's cleaned up.
		primMaterials_[primType] = pMat;

		if (pMat->isDefault()) {
			X_ERROR("Prim", "Error loading one of primative materials");
			return false;
		}
		// but i don't think a material can currently be loaded in a self contained manner.
		// since in order for it to create a render state it needs a pass state.
		// so i think when we make a material we should really classifily everything.
		// work out what the shader that is going to be used and the permatation required.
		// and then once we have that we can reflect the permatations shader and know all the cbuffers this material must have.
		// which then means we know at compile time that material shader params are valid.
		// does that mean we want to move shader compiling offline lol? :| :D twat! <:)
		// 
		// How to keep this online?
		// We would have to know the shader but at runtime we compile the shader selecte a permatation.
		// then work out the cbuffers of the shader and create a variable state for the material.
		// which i think i'll do for now, and move more offline later. 
		// 

	}

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

	for (auto& vp : vertexPages_) {
		vp.destoryVB(pRender);
	}

	for (auto& pMat : primMaterials_)
	{
		if (pMat) {
			pMaterialManager_->releaseMaterial(pMat);
		}
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

size_t PrimativeContext::maxVertsPerPrim(void) const
{
	return NUMVERTS_PER_PAGE;
}

PrimativeContext::Mode PrimativeContext::getMode(void) const
{
	return mode_;
}

void PrimativeContext::reset(void)
{
	pushBufferArr_.clear();

	currentPage_ = -1;

	for (auto& vp : vertexPages_) {
		vp.reset();
	}

	// if the compiler don't unroll this it should just kill itself..
	for (uint32_t i = 0; i < ObjectType::ENUM_COUNT; i++) {
		objects_[i].clear();
	}
}

bool PrimativeContext::isEmpty(void) const
{
	return pushBufferArr_.isEmpty();
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

	if (currentPage_ > MAX_PAGES) {
		X_ERROR("Prim", "Exceeded max pages for context.");
		// instead of crashing we just go back one page and reset it.
		// reason i don't go back to first page is because this is going to cause rendering artifacts.
		// but stuff in the later pages is less likley to be noticible.
		// * If this ever happens you are drawing far too much prim shit.
		//	 if you reall want to draw that much should just create multiple prim contexts ^^.
		//	 as each has a seperate set of pages.
		--currentPage_;
		vertexPages_[currentPage_].reset();
	}

	auto& newPage = vertexPages_[currentPage_];
	if(!newPage.isVbValid()) {
		// virgin page, need a good slapping..
		// renderSys support creating vertexBuffers in parralel from multiple threads so this is fine.
		newPage.createVB(pRender_);
	}

	return newPage;
}

PrimativeContext::PrimVertex* PrimativeContext::addPrimative(uint32_t numVertices, PrimitiveType::Enum primType,
	Material* pMaterial)
{
	X_ASSERT_ALIGNMENT(pMaterial, 8, 0); // we require 3 lsb bits for flags.

	auto& curPage = getPage(numVertices);
	auto& vertexArr = curPage.verts;

	// if the last entry was the same type
	// just merge the verts in.
	if (pushBufferArr_.isNotEmpty() && pushBufferArr_.back().material == pMaterial &&
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

PrimativeContext::ObjectParam* PrimativeContext::addObject(ObjectType::Enum type)
{
	return &objects_[type].AddOne();
}

X_NAMESPACE_END
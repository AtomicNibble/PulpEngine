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


void PrimativeContextSharedResources::CreateSphere(VertArr& vb, IndexArr& ib,
	float radius, uint32_t rings, uint32_t sections)
{
	// calc required number of vertices/indices/triangles to build a sphere for the given parameters
	uint32 numVertices, numTriangles, numIndices;

	numVertices = (rings - 1) * (sections + 1) + 2;
	numTriangles = (rings - 2) * sections * 2 + 2 * sections;
	numIndices = numTriangles * 3;

	// setup buffers
	vb.reserve(vb.size() + numVertices);
	ib.reserve(ib.size() + numIndices);

	// 1st pole vertex
	VertArr::Type vert;
	vert.pos = Vec3f(0.0f, 0.0f, radius);
	vb.emplace_back(vert);

	// calculate "inner" vertices
	const float sectionSlice = toRadians(360.0f / static_cast<float>(sections));
	const float ringSlice = toRadians(180.0f / static_cast<float>(rings));

	uint32 a, i;
	for (a = 1; a < rings; ++a)
	{
		float w = math<float>::sin(a * ringSlice);
		for (i = 0; i <= sections; ++i)
		{
			Vec3f v;
			v.x = radius * cosf(i * sectionSlice) * w;
			v.y = radius * sinf(i * sectionSlice) * w;
			v.z = radius * cosf(a * ringSlice);
			
			vert.pos = v;
			vb.emplace_back(vert);
		}
	}

	// 2nd vertex of pole (for end cap)
	vert.pos = Vec3f(0.0f, 0.0f, -radius);
	vb.emplace_back(vert);

	// build "inner" faces
	for (a = 0; a < rings - 2; ++a)
	{
		for (i = 0; i < sections; ++i)
		{
			ib.push_back(safe_static_cast<IndexArr::Type>(1 + a * (sections + 1) + i + 1));
			ib.push_back(safe_static_cast<IndexArr::Type>(1 + a * (sections + 1) + i));
			ib.push_back(safe_static_cast<IndexArr::Type>(1 + (a + 1) * (sections + 1) + i + 1));
			ib.push_back(safe_static_cast<IndexArr::Type>(1 + (a + 1) * (sections + 1) + i));
			ib.push_back(safe_static_cast<IndexArr::Type>(1 + (a + 1) * (sections + 1) + i + 1));
			ib.push_back(safe_static_cast<IndexArr::Type>(1 + a * (sections + 1) + i));
		}
	}

	// build faces for end caps (to connect "inner" vertices with poles)
	for (i = 0; i < sections; ++i)
	{
		ib.push_back(safe_static_cast<IndexArr::Type>(1 + (0) * (sections + 1) + i));
		ib.push_back(safe_static_cast<IndexArr::Type>(1 + (0) * (sections + 1) + i + 1));
		ib.push_back(safe_static_cast<IndexArr::Type>(0));
	}

	for (i = 0; i < sections; ++i)
	{
		ib.push_back(safe_static_cast<IndexArr::Type>(1 + (rings - 2) * (sections + 1) + i + 1));
		ib.push_back(safe_static_cast<IndexArr::Type>(1 + (rings - 2) * (sections + 1) + i));
		ib.push_back(safe_static_cast<IndexArr::Type>((rings - 1) * (sections + 1) + 1));
	}
}

void PrimativeContextSharedResources::CreateCone(VertArr& vb, IndexArr& ib,
	float radius, float height, uint32_t sections)
{
	// calc required number of vertices/indices/triangles to build a cone for the given parameters
	uint32 numVertices, numTriangles, numIndices;
	uint16_t i;

	numVertices = 2 * (sections + 1) + 2;
	numTriangles = 2 * sections;
	numIndices = numTriangles * 3;

	// setup buffers
	vb.reserve(vb.size() + numVertices);
	ib.reserve(ib.size() + numIndices);

	// center vertex
	VertArr::Type vert;
	vert.pos = Vec3f(0.0f, 0.0f, 0.0f);
	vb.emplace_back(vert);

	// create circle around it
	float sectionSlice = toRadians(360.0f / (float)sections);
	for (i = 0; i <= sections; ++i)
	{
		Vec3f v;
		v.x = radius * cosf(i * sectionSlice);
		v.y = 0.0f;
		v.z = radius * sinf(i * sectionSlice);
		vert.pos = v;
		vb.emplace_back(vert);
	}

	// build faces for end cap 
	for (i = 0; i < sections; ++i)
	{
		ib.push_back(0);
		ib.push_back(1 + i);
		ib.push_back(1 + i + 1);
	}

	// top
	vert.pos = Vec3f(0.0f, height, 0.0f);
	vb.emplace_back(vert);

	for (i = 0; i <= sections; ++i)
	{
		Vec3f v;
		v.x = radius * cosf(i * sectionSlice);
		v.y = 0.0f;
		v.z = radius * sinf(i * sectionSlice);

		Vec3f v1;
		v1.x = radius * cosf(i * sectionSlice + 0.01f);
		v1.y = 0.0f;
		v1.z = radius * sinf(i * sectionSlice + 0.01f);

		X_UNUSED(v1);
		// Vec3f d(v1 - v);
		// Vec3f d1(Vec3f(0.0, height, 0.0f) - v);
		// Vec3f n((d1.cross(d)).normalized());

		vert.pos = v;

		vb.emplace_back(vert);
	}

	// build faces
	for (i = 0; i < sections; ++i)
	{
		ib.emplace_back(safe_static_cast<IndexArr::Type>(sections + 2));
		ib.emplace_back(safe_static_cast<IndexArr::Type>(sections + 3 + i + 1));
		ib.emplace_back(safe_static_cast<IndexArr::Type>(sections + 3 + i));
	}
}


void PrimativeContextSharedResources::CreateCylinder(VertArr& vb, IndexArr& ib,
	float radius, float height, uint32_t sections)
{
	// calc required number of vertices/indices/triangles to build a cylinder for the given parameters
	uint32 numVertices, numTriangles, numIndices;

	numVertices = 4 * (sections + 1) + 2;
	numTriangles = 4 * sections;
	numIndices = numTriangles * 3;

	// setup buffers
	vb.reserve(vb.size() + numVertices);
	ib.reserve(ib.size() + numIndices);

	float sectionSlice = toRadians(360.0f / (float)sections);

	VertArr::Type vert;
	// bottom cap
	{
		// center bottom vertex
		vert.pos = Vec3f(0.0f, -0.5f * height, 0.0f);
		vb.emplace_back(vert);

		// create circle around it
		uint16_t i;
		for (i = 0; i <= sections; ++i)
		{
			Vec3f v;
			v.x = radius * math<float>::cos(i * sectionSlice);
			v.y = -0.5f * height;
			v.z = radius * math<float>::sin(i * sectionSlice);
			vert.pos = v;
			vb.emplace_back(vert);
		}

		// build faces
		for (i = 0; i < sections; ++i)
		{
			ib.push_back(0);
			ib.push_back(1 + i);
			ib.push_back(1 + i + 1);
		}
	}

	// side
	{
		uint16_t vIdx = safe_static_cast<uint16_t, size_t>(vb.size());

		uint32 i;
		for (i = 0; i <= sections; ++i)
		{
			Vec3f v;
			v.x = radius * math<float>::cos(i * sectionSlice);
			v.y = -0.5f * height;
			v.z = radius * math<float>::sin(i * sectionSlice);

		//	Vec3f n(v.normalized());
		//	vb.emplace_back(v, n);
		//	vb.emplace_back(Vec3f(v.x, -v.y, v.z), n);

			vert.pos = v;
			vb.emplace_back(vert);
			vert.pos = Vec3f(v.x, -v.y, v.z);
			vb.emplace_back(vert);
		}

		// build faces
		for (i = 0; i < sections; ++i, vIdx += 2)
		{
			ib.emplace_back(vIdx);
			ib.emplace_back<IndexArr::Type>(vIdx + 1);
			ib.emplace_back<IndexArr::Type>(vIdx + 2);

			ib.emplace_back<IndexArr::Type>(vIdx + 1);
			ib.emplace_back<IndexArr::Type>(vIdx + 3);
			ib.emplace_back<IndexArr::Type>(vIdx + 2);
		}
	}

	// top cap
	{
		uint16_t vIdx = safe_static_cast<IndexArr::Type>(vb.size());

		// center top vertex
		vert.pos = Vec3f(0.0f, 0.5f * height, 0.0f);
		vb.emplace_back(vert);

		// create circle around it
		uint16_t i;
		for (i = 0; i <= sections; ++i)
		{
			Vec3f v;
			v.x = radius * math<float>::cos(i * sectionSlice);
			v.y = 0.5f * height;
			v.z = radius * math<float>::sin(i * sectionSlice);
			vert.pos = v;
			vb.emplace_back(vert);
		}

		// build faces
		for (i = 0; i < sections; ++i)
		{
			ib.emplace_back(vIdx);
			ib.emplace_back<IndexArr::Type>(vIdx + 1 + i + 1);
			ib.emplace_back<IndexArr::Type>(vIdx + 1 + i);
		}
	}
}

// ---------------------------------------------------------------------------


PrimativeContextSharedResources::ShapeLod::ShapeLod()
{
	indexCount = 0;
	startIndex = 0;
	baseVertex = 0;
}

// ---------------------------------------------------------------------------


PrimativeContextSharedResources::Shape::Shape()
{
	vertexBuf = render::INVALID_BUF_HANLDE;
	indexbuf = render::INVALID_BUF_HANLDE;
}

// ---------------------------------------------------------------------------


PrimativeContextSharedResources::PrimativeContextSharedResources()
{
	core::zero_object(primMaterials_);
	core::zero_object(primMaterialsDepth_);
}


bool PrimativeContextSharedResources::init(render::IRender* pRender)
{
	X_ASSERT_NOT_NULL(pRender);
	X_ASSERT_NOT_NULL(pMaterialManager_);

	if (!loadMaterials()) {
		return false;
	}
	if (!createShapeBuffers(pRender)) {
		return false;
	}

	return true;
}

bool PrimativeContextSharedResources::loadMaterials(void)
{
	auto loadMaterials = [](PrimMaterialArr& primMaterials, const char* pSuffix) -> bool
	{
		core::StackString<assetDb::ASSET_NAME_MAX_LENGTH, char> matName;

		for (size_t i = 0; i < PrimitiveType::ENUM_COUNT; i++)
		{
			const auto primType = static_cast<PrimitiveType::Enum>(i);

			matName.clear();
			matName.appendFmt("code%c$%s%s", assetDb::ASSET_NAME_SLASH, PrimitiveType::ToString(primType), pSuffix);
			matName.toLower();

			Material* pMat = pMaterialManager_->loadMaterial(matName.c_str());

			// we still assign even tho may be default so it get's cleaned up.
			primMaterials[primType] = pMat;

			if (pMat->isDefault()) {
				X_ERROR("Prim", "Error loading one of primative materials");
				return false;
			}
		}
		return true;
	};

	if (!loadMaterials(primMaterials_, "")) {
		return false;
	}
	if (!loadMaterials(primMaterialsDepth_, "_depth")) {
		return false;
	}

	return true;
}

bool PrimativeContextSharedResources::createShapeBuffers(render::IRender* pRender)
{
	VertArr vertArr(g_3dEngineArena);
	IndexArr indexArr(g_3dEngineArena);

	// render system wants all cpu buffers 16byte aligned.
	vertArr.getAllocator().setBaseAlignment(16);
	indexArr.getAllocator().setBaseAlignment(16);
	// not really needed since we reserver for each shape.
	vertArr.setGranularity(256);
	indexArr.setGranularity(256);

	for (uint32_t i = 0; i < ShapeType::ENUM_COUNT; i++)
	{
		ShapeType::Enum type = static_cast<ShapeType::Enum>(i);
		Shape& shape = shapes_[i];

		// clear ready for this shape.
		vertArr.clear();
		indexArr.clear();

		// create all the lods.
		for (uint32_t lodIdx = 0; lodIdx < SHAPES_NUM_LOD; lodIdx++)
		{
			auto& shapeLod = shape.lods[lodIdx];
			shapeLod.baseVertex = safe_static_cast<uint16_t>(vertArr.size());
			shapeLod.startIndex = safe_static_cast<uint16_t>(indexArr.size());

			if (type == ShapeType::Sphere)
			{
				float radius = 1.f;
				uint32_t rings = 9 + (4 * lodIdx);
				uint32_t sections = 9 + (4 * lodIdx);

				CreateSphere(vertArr, indexArr, radius, rings, sections);
			}
			else if (type == ShapeType::Cone)
			{
				float radius = 1.f;
				float height = 1.f;
				uint32_t sections = 9 + (4 * lodIdx);

				CreateCone(vertArr, indexArr, radius, height, sections);
			}
			else if (type == ShapeType::Cylinder)
			{
				float radius = 1.f;
				float height = 1.f;
				uint32_t sections = 9 + (4 * lodIdx);

				CreateCylinder(vertArr, indexArr, radius, height, sections);
			}
			else
			{
				X_ASSERT_NOT_IMPLEMENTED();
				return false;
			}

			shapeLod.indexCount = safe_static_cast<uint16_t>(indexArr.size() - shapeLod.startIndex);

			if (shapeLod.indexCount < 1) {
				X_ERROR("Prim", "Shape %s lod %" PRIu32 " has index size of zero", ShapeType::ToString(type), lodIdx);
				return false;
			}
		}

		// now we make buffers for the shape.
		shape.vertexBuf = pRender->createVertexBuffer(sizeof(VertArr::Type), safe_static_cast<uint32_t>(vertArr.size()), vertArr.data(), render::BufUsage::IMMUTABLE);
		shape.indexbuf = pRender->createIndexBuffer(sizeof(IndexArr::Type), safe_static_cast<uint32_t>(indexArr.size()), indexArr.data(), render::BufUsage::IMMUTABLE);

		if (shape.vertexBuf == render::INVALID_BUF_HANLDE) {
			X_ERROR("Prim", "Failed to create vertex buffer");
			return false;
		}
		if (shape.indexbuf == render::INVALID_BUF_HANLDE) {
			X_ERROR("Prim", "Failed to create index buffer");
			return false;
		}
	}

	return true;
}

void PrimativeContextSharedResources::releaseResources(render::IRender* pRender)
{
	// shapes.
	for (auto& shape : shapes_)
	{
		if (shape.vertexBuf != render::INVALID_BUF_HANLDE) {
			pRender->destoryVertexBuffer(shape.vertexBuf);
		}
		if (shape.indexbuf != render::INVALID_BUF_HANLDE) {
			pRender->destoryIndexBuffer(shape.indexbuf);
		}

		shape.vertexBuf = render::INVALID_BUF_HANLDE;
		shape.indexbuf = render::INVALID_BUF_HANLDE;
	}

	// materials
	auto releaseMaterials = [](PrimMaterialArr& primMaterials) {
		for (auto& pMat : primMaterials)
		{
			if (pMat) {
				pMaterialManager_->releaseMaterial(pMat);
			}
		}

		core::zero_object(primMaterials);
	};

	releaseMaterials(primMaterials_);
	releaseMaterials(primMaterialsDepth_);
}

// ---------------------------------------------------------------------------

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

PrimativeContext::PrimativeContext(PrimativeContextSharedResources& sharedRes, Mode mode, core::MemoryArenaBase* arena) :
	pushBufferArr_(arena),
	vertexPages_(arena, MAX_PAGES, arena),
	depthPrim_(0),
	currentPage_(-1),
	mode_(mode),
	objectArrays_{ arena, arena, arena } ,
	sharedRes_(sharedRes)
{
	pushBufferArr_.reserve(64);
	pushBufferArr_.setGranularity(512);

	for (auto& objectArr : objectArrays_)
	{
		objectArr.setGranularity(256);
	}

	X_ASSERT(vertexPages_.isNotEmpty(), "Must have atleast one vertex page")();
	// for the fist page we reverse something small
	// before setting the large granularity.
	// that way we grow fast but if not rendering much we stay small.
}


PrimativeContext::~PrimativeContext() 
{

}


bool PrimativeContext::freePages(render::IRender* pRender)
{
	for (auto& vp : vertexPages_) {
		vp.destoryVB(pRender);
	}

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
		objectArrays_[i].clear();
	}
}

void PrimativeContext::setDepthTest(bool enabled)
{
	depthPrim_ = enabled ? 1 : 0;
}

bool PrimativeContext::isEmpty(void) const
{
	if (pushBufferArr_.isNotEmpty()) {
		return false;
	}

	for (const auto& objectArr : objectArrays_)
	{
		if (objectArr.isNotEmpty()) {
			return false;
		}
	}

	return true;
}

const PrimativeContext::PushBufferArr& PrimativeContext::getUnsortedBuffer(void) const
{
	return pushBufferArr_;
}

const PrimativeContext::ObjectTypesParamArr& PrimativeContext::getObjectArrayBuffers(void) const
{
	return objectArrays_;
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
	auto* pMat = depthPrim_ ? sharedRes_.getMaterialDepthTest(primType) : sharedRes_.getMaterial(primType);

	return addPrimative(numVertices, primType, pMat);
}

PrimativeContext::ObjectParam* PrimativeContext::addObject(ObjectType::Enum type)
{
	return &objectArrays_[type].AddOne();
}

X_NAMESPACE_END
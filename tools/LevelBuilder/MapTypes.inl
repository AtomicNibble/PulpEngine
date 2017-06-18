#pragma once


X_NAMESPACE_BEGIN(mapfile)


XMapPrimitive::XMapPrimitive(PrimType::Enum type) :
	type_(type) 
{
}

XMapPrimitive::~XMapPrimitive(void) 
{

}

PrimType::Enum XMapPrimitive::getType(void) const
{
	return type_; 
}

const core::string& XMapPrimitive::getLayer(void) const 
{ 
	return layer_; 
}

const bool XMapPrimitive::hasLayer(void) const 
{ 
	return layer_.isNotEmpty(); 
}


// ======================

XMapBrushSide::MaterialInfo::MaterialInfo(void)
{
	rotate = 0.f;
}

// ======================

XMapBrushSide::XMapBrushSide(void)
{

}

XMapBrushSide::~XMapBrushSide(void)
{

}

const char*	XMapBrushSide::GetMaterialName(void) const 
{ 
	return material_.name.c_str(); 
}

const Planef& XMapBrushSide::GetPlane(void) const 
{ 
	return plane_; 
}

X_INLINE const XMapBrushSide::MaterialInfo& XMapBrushSide::GetMaterial(void) const
{
	return material_;
}

X_INLINE const XMapBrushSide::MaterialInfo& XMapBrushSide::GetLightMap(void) const
{
	return lightMap_;
}

X_INLINE void XMapBrushSide::SetPlane(const Planef& plane)
{
	plane_ = plane;
}

// ======================


XMapBrush::XMapBrush(core::MemoryArenaBase* arena) : 
	XMapPrimitive(PrimType::BRUSH), 
	sides_(arena)
{ 
	sides_.reserve(6); 
}

XMapBrush::~XMapBrush(void)
{

}

size_t XMapBrush::GetNumSides(void) const 
{ 
	return sides_.size(); 
}

void XMapBrush::AddSide(XMapBrushSide* pSide)
{ 
	sides_.push_back(pSide);
}

XMapBrushSide* XMapBrush::GetSide(size_t i) const 
{ 
	return sides_[i]; 
}


// ======================


void XMapPatch::SetHorzSubdivisions(size_t num)
{ 
	horzSubdivisions_ = num; 
}

void XMapPatch::SetVertSubdivisions(size_t num)
{ 
	vertSubdivisions_ = num;
}

size_t XMapPatch::GetHorzSubdivisions(void) const
{ 
	return horzSubdivisions_; 
}

size_t XMapPatch::GetVertSubdivisions(void) const
{ 
	return vertSubdivisions_; 
}

size_t XMapPatch::GetNumIndexes(void) const
{ 
	return indexes_.size(); 
}

const int* XMapPatch::GetIndexes(void) const
{ 
	return indexes_.ptr(); 
}

const xVert& XMapPatch::operator[](const int idx) const
{
	return verts_[idx];
}

xVert& XMapPatch::operator[](const int idx)
{
	return verts_[idx];
}

void XMapPatch::SetMesh(bool b)
{
	isMesh_ = b;
}

const bool XMapPatch::isMesh(void) const
{
	return isMesh_;
}

const char* XMapPatch::GetMatName(void) const
{
	return matName_.c_str();
}


// ======================

IgnoreList::IgnoreList(const IgnoreArray& ignoreList)
	: ignoreList_(ignoreList) 
{
}

IgnoreList::IgnoreList(IgnoreArray&& ignoreList)
	: ignoreList_(std::move(ignoreList))
{
}

bool IgnoreList::isIgnored(const core::string& layerName) const
{
	for (const auto& name : ignoreList_)
	{
		if (name == layerName) {
			return true;
		}
	}
	return false;
}

// ======================

XMapEntity::XMapEntity(core::MemoryArenaBase* arena, core::MemoryArenaBase* primArena) :
	primArena_(X_ASSERT_NOT_NULL(arena)),
	primitives_(X_ASSERT_NOT_NULL(arena))
{

}

XMapEntity::~XMapEntity(void) 
{
	for (size_t i = 0; i < primitives_.size(); i++)
	{
		X_DELETE(primitives_[i], primArena_);
	}

	primitives_.free();
}

size_t XMapEntity::GetNumPrimitives(void) const 
{ 
	return primitives_.size(); 
}

XMapPrimitive* XMapEntity::GetPrimitive(size_t i) const
{ 
	return primitives_[i]; 
}
 
void XMapEntity::AddPrimitive(XMapPrimitive* pPrim) 
{ 
	primitives_.push_back(pPrim);
}

// ======================


X_NAMESPACE_END

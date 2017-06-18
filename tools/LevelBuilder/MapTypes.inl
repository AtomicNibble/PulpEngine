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

XMapBrushSide::XMapBrushSide(void)
{

}

XMapBrushSide::~XMapBrushSide(void)
{

}

const char*	XMapBrushSide::GetMaterialName(void) const 
{ 
	return material.name.c_str(); 
}

const Planef& XMapBrushSide::GetPlane(void) const 
{ 
	return plane; 
}

XMapBrushSide::MaterialInfo::MaterialInfo(void)
{
	rotate = 0.f;
}

// ======================


XMapBrush::XMapBrush(void) : 
	XMapPrimitive(PrimType::BRUSH), 
	sides(g_arena)
{ 
	sides.reserve(6); 
}

XMapBrush::~XMapBrush(void)
{

}

size_t XMapBrush::GetNumSides(void) const 
{ 
	return sides.size(); 
}

void XMapBrush::AddSide(XMapBrushSide* pSide)
{ 
	sides.push_back(pSide);
}

XMapBrushSide* XMapBrush::GetSide(size_t i) const 
{ 
	return sides[i]; 
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

XMapEntity::XMapEntity(void) : 
	primArena_(nullptr),
	primitives(g_arena)
{

}

XMapEntity::~XMapEntity(void) 
{
	X_ASSERT_NOT_NULL(primArena_);

	for (size_t i = 0; i < primitives.size(); i++)
	{
		X_DELETE(primitives[i], primArena_);
	}

	primitives.free();
}

size_t XMapEntity::GetNumPrimitives(void) const 
{ 
	return primitives.size(); 
}

XMapPrimitive* XMapEntity::GetPrimitive(size_t i) const
{ 
	return primitives[i]; 
}
 
void XMapEntity::AddPrimitive(XMapPrimitive* pPrim) 
{ 
	primitives.push_back(pPrim);
}

// ======================


X_NAMESPACE_END

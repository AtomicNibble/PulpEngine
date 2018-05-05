#pragma once

X_NAMESPACE_BEGIN(level)

namespace mapFile
{
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

    const char* XMapBrushSide::GetMaterialName(void) const
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

    XMapBrush::XMapBrush(core::MemoryArenaBase* arena, core::MemoryArenaBase* primArena) :
        XMapPrimitive(PrimType::BRUSH),
        primArena_(primArena),
        sides_(arena)
    {
        sides_.reserve(6);
    }

    XMapBrush::~XMapBrush(void)
    {
        for (auto* pSide : sides_) {
            X_DELETE(pSide, primArena_);
        }
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

    void XMapPatch::SetHorzSubdivisions(int32_t num)
    {
        horzSubdivisions_ = num;
    }

    void XMapPatch::SetVertSubdivisions(int32_t num)
    {
        vertSubdivisions_ = num;
    }

    int32_t XMapPatch::GetHorzSubdivisions(void) const
    {
        return horzSubdivisions_;
    }

    int32_t XMapPatch::GetVertSubdivisions(void) const
    {
        return vertSubdivisions_;
    }

    int32_t XMapPatch::GetNumIndexes(void) const
    {
        return safe_static_cast<int32_t>(indexes_.size());
    }

    const int* XMapPatch::GetIndexes(void) const
    {
        return indexes_.ptr();
    }

    const LvlVert& XMapPatch::operator[](const int idx) const
    {
        return verts_[idx];
    }

    LvlVert& XMapPatch::operator[](const int idx)
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

    IgnoreList::IgnoreList(const IgnoreArray& ignoreList) :
        ignoreList_(ignoreList)
    {
    }

    IgnoreList::IgnoreList(IgnoreArray&& ignoreList) :
        ignoreList_(std::move(ignoreList))
    {
    }

    void IgnoreList::add(const core::string& layerName)
    {
        ignoreList_.insertSorted(layerName);
    }

    bool IgnoreList::isIgnored(const core::string& layerName) const
    {
        if (ignoreList_.isEmpty()) {
            return false;
        }

        return ignoreList_.findSorted(layerName) != ignoreList_.end();
    }

    // ======================

    XMapEntity::XMapEntity(core::MemoryArenaBase* arena, core::MemoryArenaBase* primArena) :
        arena_(X_ASSERT_NOT_NULL(arena)),
        primArena_(X_ASSERT_NOT_NULL(primArena)),
        primitives_(X_ASSERT_NOT_NULL(arena)),
        epairs(arena)
    {
        core::zero_object(primCounts_);
    }

    XMapEntity::~XMapEntity(void)
    {
        for (size_t i = 0; i < primitives_.size(); i++) {
            X_DELETE(primitives_[i], primArena_);
        }

        primitives_.free();
    }

    size_t XMapEntity::GetNumPrimitives(void) const
    {
        return primitives_.size();
    }

    X_INLINE const XMapEntity::PrimTypeNumArr& XMapEntity::getPrimCounts(void) const
    {
        X_ASSERT(core::accumulate(primCounts_.begin(), primCounts_.end(), 0_sz) == primitives_.size(),
            "Primatives size and counts don't match")
        ();

        return primCounts_;
    }

    XMapPrimitive* XMapEntity::GetPrimitive(size_t i) const
    {
        return primitives_[i];
    }

    void XMapEntity::AddPrimitive(XMapPrimitive* pPrim)
    {
        ++primCounts_[pPrim->getType()];
        primitives_.push_back(pPrim);
    }

    // ======================

} // namespace mapFile

X_NAMESPACE_END

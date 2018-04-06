

X_NAMESPACE_BEGIN(engine)

size_t World3D::numAreas(void) const
{
    return areas_.size();
}

size_t World3D::numPortalsInArea(int32_t areaNum) const
{
    X_ASSERT((areaNum >= 0 && areaNum < safe_static_cast<int32_t>(numAreas())),
        "areaNum out of range")
    (areaNum, numAreas());

    return areas_[areaNum].portals.size();
}

bool World3D::areaHasPortals(int32_t areaNum) const
{
    return numPortalsInArea(areaNum) > 0;
}

bool World3D::isPointInAnyArea(const Vec3f& pos) const
{
    int32_t areaOut;
    return isPointInAnyArea(pos, areaOut);
}

bool World3D::isCamArea(int32_t areaNum) const
{
    return areaNum == camArea_;
}

bool World3D::isAreaVisible(int32_t areaNum) const
{
    int32_t index = areaNum / 32;
    uint32_t bit = areaNum % 32;

    return core::bitUtil::IsBitSet(visibleAreaFlags_[index], bit);
}

bool World3D::isAreaVisible(const Area& area) const
{
    return isAreaVisible(area.areaNum);
}

X_NAMESPACE_END

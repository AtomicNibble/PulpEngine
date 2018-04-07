#include "stdafx.h"
#include "Compiler.h"

#include <Time\StopWatch.h>
#include <Hashing\crc32.h>
#include <String\HumanDuration.h>
#include <String\Json.h>

#include <IFileSys.h>
#include <ITimer.h>
#include <IConsole.h>

#include "LvlFmts\LvlSource.h"
#include "LvlFmts/mapFile\MapFile.h"
#include "LvlFmts/mapFile/Loader.h"
#include "LvlBuilder.h"

#include "Model/ModelCache.h"
#include "Material/MaterialManager.h"

X_NAMESPACE_BEGIN(level)

namespace
{
    void WindingToTri(const Winding* pWinding, core::Array<Vec3f>& verts)
    {
        const Winding* w = X_ASSERT_NOT_NULL(pWinding);

        size_t numPoints = w->getNumPoints();

        verts.clear();
        verts.reserve((numPoints - 2) * 3);

        for (size_t i = 2; i < numPoints; i++) {
            for (size_t j = 0; j < 3; j++) {
                Vec3f& vert = verts.AddOne();

                if (j == 0) {
                    vert = w->at(0).asVec3();
                }
                else if (j == 1) {
                    vert = w->at(i - 1).asVec3();
                }
                else {
                    vert = w->at(i).asVec3();
                }
            }
        }
    }

    bool savePortalWinding(const Winding* pWinding, core::ByteStream& stream)
    {
        X_ASSERT_NOT_NULL(pWinding);

        // we save the winding, plane and debug verts
        core::Array<Vec3f> verts(g_arena);
        Planef plane;

        pWinding->getPlane(plane);
        WindingToTri(pWinding, verts);

        stream.write(safe_static_cast<uint32_t>(pWinding->getNumPoints()));
        stream.write(pWinding->begin(), pWinding->getNumPoints());
        stream.write(plane);
        stream.write(safe_static_cast<uint32_t>(verts.size()));
        stream.write(verts.ptr(), verts.size());
        return true;
    }

    class JsonByteBuffer
    {
    public:
        typedef char Ch;

    public:
        JsonByteBuffer(core::ByteStream& stream) :
            stream_(stream)
        {
        }

        JsonByteBuffer(JsonByteBuffer&& rhs) :
            stream_(std::move(rhs.stream_))
        {
        }

        JsonByteBuffer& operator=(JsonByteBuffer&& rhs)
        {
            stream_ = std::move(rhs.stream_);
            return *this;
        }

        void Put(Ch c)
        {
            stream_.write(c);
        }
        void PutUnsafe(Ch c)
        {
            stream_.write(c);
        }

        void Flush(void)
        {
            // ...
        }

    private:
        core::ByteStream& stream_;
    };

    struct JsonByteStreamWriter
    {
        JsonByteStreamWriter(core::MemoryArenaBase* arena) :
            stream(arena),
            jsonBuf(stream),
            writer(jsonBuf)
        {
        }

        core::ByteStream stream;
        JsonByteBuffer jsonBuf;
        core::json::Writer<JsonByteBuffer> writer;
    };

} // namespace

core::MemoryArenaBase* g_bspFaceArena = nullptr;
core::MemoryArenaBase* g_bspPortalArena = nullptr;
core::MemoryArenaBase* g_bspNodeArena = nullptr;
core::MemoryArenaBase* g_windingArena = nullptr;
core::MemoryArenaBase* g_windingPointsArena = nullptr;

Compiler::Compiler(core::MemoryArenaBase* arena, physics::IPhysicsCooking* pPhysCooking) :
    arena_(arena),
    planes_(arena),
    pPhysCooking_(X_ASSERT_NOT_NULL(pPhysCooking)),
    bspFaceAllocator_(sizeof(bspFace), X_ALIGN_OF(bspFace), 1 << 20, core::VirtualMem::GetPageSize() * 8), // grow 32k at a time.
    bspPortalAllocator_(core::Max(sizeof(bspPortal), sizeof(Winding)), core::Max(X_ALIGN_OF(bspPortal), X_ALIGN_OF(Winding)), 1 << 20, core::VirtualMem::GetPageSize() * 8),
    bspNodeAllocator_(sizeof(bspNode), X_ALIGN_OF(bspNode), 1 << 20, core::VirtualMem::GetPageSize() * 8),
    windingDataAllocator_((1 << 20) * 10, core::VirtualMem::GetPageSize() * 8, 16, WindingDataArena::getMemoryOffsetRequirement() + 4),
    windingDataArena_(&windingDataAllocator_, "WindingDataArena"),

    areas_(arena),
    lights_(arena),
    staticModels_(arena),
    multiModelRefLists_{
        X_PP_REPEAT_COMMA_SEP(8, arena)},
    stringTable_(arena)
{
    pModelCache_ = X_NEW(ModelCache, arena, "LvlModelCache")(arena);
    pMaterialMan_ = X_NEW(MatManager, arena, "LvlMaterialMan")(arena);

    // set the pointers.
    g_bspFaceArena = &bspFaceAllocator_.arena_;
    g_bspPortalArena = &bspPortalAllocator_.arena_;
    g_bspNodeArena = &bspNodeAllocator_.arena_;
    g_windingArena = &bspPortalAllocator_.arena_;
    g_windingPointsArena = &windingDataArena_;
}

Compiler::~Compiler()
{
    g_bspFaceArena = nullptr;
    g_bspPortalArena = nullptr;
    g_bspNodeArena = nullptr;
    g_windingArena = nullptr;
    g_windingPointsArena = nullptr;

    if (pMaterialMan_) {
        pMaterialMan_->ShutDown();
    }

    X_DELETE(pModelCache_, arena_);
    X_DELETE(pMaterialMan_, arena_);
}

bool Compiler::init(void)
{
    if (!pMaterialMan_->Init()) {
        return false;
    }

    if (!pModelCache_->loadDefaultModel()) {
        return false;
    }

    return true;
}

bool Compiler::compileLevel(core::Path<char>& path, core::Path<char>& outPath)
{
    if (core::strUtil::IsEqualCaseInsen("map", path.extension())) {
        X_ERROR("Map", "extension is not valid, must be .map");
        return false;
    }

    X_LOG0("Map", "Loading: \"%s\"", path.fileName());
    core::StopWatch stopwatch;

    mapFile::MapFileSource source(g_arena, *pModelCache_, *pMaterialMan_, planes_);
    if (!source.load(path)) {
        X_ERROR("Map", "Failed to load source");
        return false;
    }

    auto& ents = source.getEntsArr();
    if (ents.isEmpty()) {
        X_ERROR("Lvl", "Map has zero entites, atleast one is required");
        return false;
    }

    source.printInfo();

    if (!processModels(ents)) {
        return false;
    }

    core::HumanDuration::Str durStr;
    X_LOG0("Info", "Compile time: ^6%s", core::HumanDuration::toString(durStr, stopwatch.GetMilliSeconds()));
    stopwatch.Start();

    if (!save(ents, outPath)) {
        return false;
    }

    X_LOG0("Info", "Save time: ^6%s", core::HumanDuration::toString(durStr, stopwatch.GetMilliSeconds()));
    return true;
}

bool Compiler::processModels(LvlEntsArr& ents)
{
    for (size_t i = 0; i < ents.size(); i++) {
        LvlEntity& entity = ents[i];
        if (entity.brushes.isEmpty()) {
            continue;
        }

        X_LOG0("Entity", "^5processing entity %" PRIuS, i);

        if (i == 0) {
            // return false if leak.
            if (!processWorldModel(ents, entity)) {
                return false;
            }
        }
        else {
            if (!processModel(entity)) {
                X_ERROR("Entity", "Failed to process entity: %" PRIuS, i);

                return false;
            }
        }
    }

    return true;
}

bool Compiler::processModel(LvlEntity& ent)
{
    X_ASSERT_NOT_IMPLEMENTED();
    return false;
}

bool Compiler::processWorldModel(LvlEntsArr& ents, LvlEntity& ent)
{
    if (ent.classType != game::ClassType::WORLDSPAWN) {
        X_ERROR("Lvl", "World model is missing class name: 'worldspawn'");
        return false;
    }

    if (!ent.MakeStructuralFaceList()) {
        return false;
    }

    if (!ent.FacesToBSP(planes_)) {
        return false;
    }

    if (!ent.MakeTreePortals(planes_)) {
        return false;
    }

    if (!ent.FilterBrushesIntoTree(planes_)) {
        return false;
    }

    if (!ent.FloodEntities(planes_, ents)) {
        X_ERROR("Lvl", "leaked");
        return false;
    }

    if (!ent.FillOutside()) {
        return false;
    }

    if (!ent.ClipSidesByTree(planes_)) {
        return false;
    }

    if (!ent.FloodAreas()) {
        return false;
    }

    if (!createAreasForPrimativates(ent)) {
        return false;
    }

    if (!ent.PruneNodes()) {
        return false;
    }

    if (!createStaticModelList(ent, ents)) {
        return false;
    }

    if (!createLightList(ents)) {
        return false;
    }

    if (!createCollisionData(ent)) {
        return false;
    }

    return true;
}

bool Compiler::createAreasForPrimativates(LvlEntity& ent)
{
    X_LOG0("Lvl", "^5addEntsPrimativesToAreas");

    if (ent.numAreas < 1) {
        X_ERROR("Lvl", "Ent has no areas");
        return false;
    }

    areas_.clear();
    areas_.resize(ent.numAreas);

    for (auto& area : areas_) {
        area.areaBegin();
    }

    for (size_t i = 0; i < ent.brushes.size(); i++) {
        LvlBrush& brush = ent.brushes[i];
        // for each side that's visable.

        for (size_t j = 0; j < brush.sides.size(); j++) {
            LvlBrushSide& side = brush.sides[j];
            if (!side.pVisibleHull) {
                continue;
            }

            auto* pMaterial = side.matInfo.pMaterial;
            if (!pMaterial) {
                X_WARNING("Lvl", "side without a material");
                continue;
            }

            // skip none visable materials.
            if (!pMaterial->isDrawn()) {
                X_LOG1("Lvl", "Skipping visible face, material not drawn: \"%s\"", side.matInfo.name.c_str());
                continue;
            }

            putWindingIntoAreas_r(side.pVisibleHull, side, ent.bspTree_.pHeadnode);
        }
    }

    for (size_t i = 0; i < ent.patches.size(); i++) {
        LvlTris& tris = ent.patches[i];

        // AddMapTriToAreas(ent, planes_, tris);
    }

    for (size_t i = 0; i < areas_.size(); i++) {
        if (!areas_[i].areaEnd(stringTable_)) {
            X_ERROR("Lvl", "Area %" PRIuS " invalid", i);
            return false;
        }

        if (!areas_[i].model.belowLimits()) {
            X_ERROR("Lvl", "Area %" PRIuS " exceeds the limits", i);
            return false;
        }
    }

    return true;
}

void Compiler::putWindingIntoAreas_r(Winding* pWinding, LvlBrushSide& side, bspNode* pNode)
{
    if (!pWinding) {
        return;
    }

    if (pNode->planenum != PLANENUM_LEAF) {
        if (side.planenum == pNode->planenum) {
            putWindingIntoAreas_r(pWinding, side, pNode->children[Side::FRONT]);
            return;
        }
        if (side.planenum == (pNode->planenum ^ 1)) {
            putWindingIntoAreas_r(pWinding, side, pNode->children[Side::BACK]);
            return;
        }

        Winding *pFront, *pBack;

        pWinding->Split(planes_[pNode->planenum], ON_EPSILON, &pFront, &pBack, g_windingArena);

        putWindingIntoAreas_r(pFront, side, pNode->children[Side::FRONT]);
        putWindingIntoAreas_r(pBack, side, pNode->children[Side::BACK]);

        if (pFront) {
            X_DELETE(pFront, g_windingArena);
        }
        if (pBack) {
            X_DELETE(pBack, g_windingArena);
        }

        return;
    }

    // if opaque leaf,  don't add
    if (pNode->opaque) {
        return;
    }

    X_ASSERT(pNode->area != -1 && pNode->area < safe_static_cast<int32_t>(areas_.size()), "Leaf node has invalid area")(pNode->area); 

    LvlArea& area = areas_[pNode->area];
    area.addWindingForSide(planes_, side, pWinding);
}

void Compiler::areasForBounds_r(AreaIdArr& areaList, const Sphere& sphere,
    const Vec3f boundsPoints[8], bspNode* pNode)
{
    X_ASSERT_NOT_NULL(boundsPoints);
    bspNode* pCurNode = X_ASSERT_NOT_NULL(pNode);

    if (pCurNode->IsAreaLeaf()) {
        int32_t areaIdx = pCurNode->area;

        // check if duplicate.
        if (areaList.isEmpty()) {
            areaList.append(areaIdx);
        }
        else {
            // just linear search as it will be fastest with such low numbers.
            // and contigous memory.
            size_t i;
            for (i = 0; i < areaList.size(); i++) {
                if (areaList[i] == areaIdx) {
                    break;
                }
            }
            if (i == areaList.size()) {
                areaList.append(areaIdx);
            }
        }

        return;
    }

    const Planef& plane = planes_[pCurNode->planenum];
    float sd = plane.distance(sphere.center());

    if (sd >= sphere.radius()) {
        pCurNode = pCurNode->children[Side::FRONT];
        if (!pCurNode->IsSolidLeaf()) {
            areasForBounds_r(areaList, sphere, boundsPoints, pCurNode);
        }
        return;
    }
    if (sd <= -sphere.radius()) {
        pCurNode = pCurNode->children[Side::BACK];
        if (!pCurNode->IsSolidLeaf()) {
            areasForBounds_r(areaList, sphere, boundsPoints, pCurNode);
        }
        return;
    }

    // check bounds points.
    bool front = false;
    bool back = false;
    for (size_t i = 0; i < 8; i++) {
        float d = plane.distance(boundsPoints[i]);

        if (d >= 0.0f) {
            front = true;
        }
        else if (d <= 0.0f) {
            back = true;
        }
        if (back && front) {
            break;
        }
    }

    if (front) {
        bspNode* frontChild = pCurNode->children[Side::FRONT];
        if (!frontChild->IsSolidLeaf()) {
            areasForBounds_r(areaList, sphere, boundsPoints, frontChild);
        }
    }
    if (back) {
        bspNode* backChild = pCurNode->children[Side::BACK];
        if (!backChild->IsSolidLeaf()) {
            areasForBounds_r(areaList, sphere, boundsPoints, backChild);
        }
    }
}

bool Compiler::createLightList(LvlEntsArr& ents)
{
    for (size_t i = 0; i < ents.size(); i++) {
        auto& ent = ents[i];
        if (ent.classType != game::ClassType::LIGHT) {
            continue;
        }

        level::Light l;
        l.pos = ent.origin;
        l.angle = Quatf(toRadians(ent.angle.x), toRadians(ent.angle.y), toRadians(ent.angle.z));
        l.type = level::LightType::Point;

        {
            auto it = ent.epairs.find(X_CONST_STRING("_color"));
            if (it == ent.epairs.end()) {
                X_WARNING("Entity", "light missing '_color' kvp at: (^8%g,%g,%g^7)",
                    ent.origin[0], ent.origin[1], ent.origin[2]);
                continue;
            }

            const core::string& str = it->second;

            if (!Color::fromString(str.begin(), str.end(), l.col)) {
                X_WARNING("Entity", "Failed to parse light color kvp \"%s\" at: (^8%g,%g,%g^7)",
                    str.c_str(), ent.origin[0], ent.origin[1], ent.origin[2]);
                continue;
            }
        }

        lights_.append(l);
    }

    return true;
}

bool Compiler::createStaticModelList(LvlEntity& worldEnt, LvlEntsArr& ents)
{
    AreaIdArr areaList(g_arena);
    areaList.reserve(areas_.size());

    for (size_t i = 0; i < level::MAP_MAX_MULTI_REF_LISTS; i++) {
        multiModelRefLists_[i].clear();
    }

    staticModels_.reserve(ents.size() / 2);

    Vec3f boundsPoints[8];

    for (size_t i = 0; i < ents.size(); i++) {
        auto& ent = ents[i];
        if (ent.classType != game::ClassType::MISC_MODEL) {
            continue;
        }

        uint32_t modelId = safe_static_cast<uint32_t>(staticModels_.size());
        level::FileStaticModel& sm = staticModels_.AddOne();

        sm.pos = ent.origin;
        sm.angle = Quatf(toRadians(ent.angle.x), toRadians(ent.angle.y), toRadians(ent.angle.z));
        // lvl ent bounds are the model bounds when type is MISC_MODEL
        sm.boundingBox = ent.bounds;

        // if the angle is not zero we need to slap a duck.
        // aka update the AABB to still contain the model when rotated.
        // axis rotations make me horny.
        if (sm.angle != Quatf::identity()) {
            // this is a quick sloppy AABB
            // loading the model and processing it's verts is required for snug AABB.
            // I will add it as a release compile option, since it will not be quick for a map with 2000 models.
            OBB obb(sm.angle, sm.boundingBox);
            sm.boundingBox = AABB(obb);
        }

        // make them world bounds.
        sm.boundingBox.move(sm.pos);

        {
            auto it = ent.epairs.find(X_CONST_STRING("model"));
            if (it == ent.epairs.end()) {
                X_WARNING("Entity", "misc_model missing 'model' kvp at: (^8%g,%g,%g^7)",
                    ent.origin[0], ent.origin[1], ent.origin[2]);
                continue;
            }

            const core::string& modelName = it->second;
            X_LOG1("Entity", "Ent model: \"%s\"", modelName.c_str());

            sm.modelNameIdx = stringTable_.addStringUnqiue(modelName.c_str());
        }

        // make a sphere, for quicker testing.
        Sphere worldSphere(sm.boundingBox);

        // get the the worldBounds points for more accurate testing if sphere test pass.
        sm.boundingBox.toPoints(boundsPoints);

        // clear from last time.
        areaList.clear();

        // traverse the world ent's tree
        areasForBounds_r(areaList, worldSphere, boundsPoints, worldEnt.bspTree_.pHeadnode);

        size_t numRefs = areaList.size();
        if (numRefs == 0) {
            // ent not in any area.
            X_ERROR("Lvl", "Entity(%" PRIuS ") does not reside in any area: (%g,%g,%g)",
                i, ent.origin.x, ent.origin.y, ent.origin.z);
            return false;
        }

        X_LOG1("Lvl", "Entity(%" PRIuS ") has %" PRIuS " refs", i, numRefs);

        // ok so we hold a list of unique areas ent is in.
        if (numRefs == 1) {
            // add to area's ref list.
            LvlArea& area = areas_[areaList[0]];
            area.modelsRefs.push_back(modelId);
        }
        else {
            // added to the multiAreaRefList.
            uint32_t flags[level::MAP_MAX_MULTI_REF_LISTS] = {0};

            auto it = areaList.begin();
            for (; it != areaList.end(); ++it) {
                const int32_t areaIdx = *it;
                // work out what area list.
                const size_t areaListIdx = (areaIdx / 32);

                flags[areaListIdx] |= (1 << (areaIdx % 32));
            }

            level::MultiAreaEntRef entRef;
            entRef.modelId = modelId;
            for (size_t x = 0; x < level::MAP_MAX_MULTI_REF_LISTS; x++) {
                if (flags[x] != 0) {
                    entRef.flags = flags[x];
                    multiModelRefLists_[x].append(entRef);
                }
            }
        }
    }

    return true;
}

bool Compiler::createCollisionData(LvlEntity& ent)
{
    auto& area = areas_[0];

    physics::GroupFlags flags;

    auto& bucket = area.collision.getBucket(flags);

    for (size_t i = 0; i < ent.brushes.size(); i++) {
        LvlBrush& brush = ent.brushes[i];

        if (!brush.opaque) {
            continue;
        }

        if (brush.isRectangle()) {
            // see if can be represented with aabb.
            bucket.addAABB(brush.bounds);
        }
        else {
            ColConvexMeshData triMesh(g_arena);

            // we need to make a tri mesh for the sides windings.
            triMesh.addBrush(brush);

            if (!triMesh.cook(pPhysCooking_)) {
                return false;
            }

            bucket.addConvexMesh(std::move(triMesh));
        }
    }

    return true;
}

bool Compiler::save(const LvlEntsArr& ents, core::Path<char>& path)
{
    std::array<core::ByteStream, FileNodes::ENUM_COUNT> nodeStreams{
        X_PP_REPEAT_COMMA_SEP(10, g_arena)};

    for (uint32_t i = 0; i < FileNodes::ENUM_COUNT; i++) {
        nodeStreams[i].reserve(1024);
    }

    FileHeader hdr;
    core::zero_object(hdr);
    hdr.fourCC = LVL_FOURCC_INVALID;
    hdr.version = LVL_VERSION;
    hdr.datacrc32 = 0;
    hdr.modified = core::DateTimeStampSmall::systemDateTime();
    hdr.numStrings = safe_static_cast<uint32_t>(stringTable_.numStrings());

    const LvlEntity& worldEnt = ents[0];

    // string table
    {
        auto& stream = nodeStreams[FileNodes::STRING_TABLE];

        core::XFileByteStream file(stream);

        if (!stringTable_.SSave(&file)) {
            X_ERROR("Lvl", "Failed to save string table");
            return false;
        }
    }

    // areas
    {
        auto& stream = nodeStreams[FileNodes::AREA_MODELS];
        const size_t requiredBytes = core::accumulate(areas_.begin(), areas_.end(), 0_sz, [](const LvlArea& area) {
            return area.model.serializeSize();
        });

        stream.reserve(requiredBytes);
        for (const auto& area : areas_) {
            area.model.writeToStream(stream);
        }
    }

    // area portals
    if (worldEnt.interPortals.isNotEmpty()) {
        auto& stream = nodeStreams[FileNodes::AREA_PORTALS];

        hdr.flags.Set(LevelFileFlags::INTER_AREA_INFO);
        hdr.flags.Set(LevelFileFlags::DEBUG_PORTAL_DATA);

        hdr.numinterAreaPortals = safe_static_cast<uint32_t>(worldEnt.interPortals.size());

        // need to write the area's
        // and the winding.
        for (const auto& iap : worldEnt.interPortals) {
            stream.write(iap.area0);
            stream.write(iap.area1);

            X_ASSERT_NOT_NULL(iap.pSide);
            X_ASSERT_NOT_NULL(iap.pSide->pWinding);

            const Winding* pWind = iap.pSide->pWinding;
            auto windRev = core::UniquePointer<Winding>(g_windingArena, pWind->ReverseWinding(g_windingArena));

            if (!savePortalWinding(pWind, stream) || !savePortalWinding(windRev.get(), stream)) {
                X_ERROR("Lvl", "Failed to save inter portal info");
                return false;
            }
        }
    }

#if 0
	// area ent ref data
	{
		auto& stream = nodeStreams[FileNodes::AREA_ENT_REFS];
		hdr.flags.Set(LevelFileFlags::AREA_ENT_REF_LISTS);

		uint32_t num = 0;

		for (auto& area : areas_)
		{
			FileAreaRefHdr refHdr;
			refHdr.startIndex = num;
			refHdr.num = safe_static_cast<uint32_t>(area.entRefs.size());

			num += safe_static_cast<uint32_t>(area.entRefs.size());

			stream.write(refHdr);
		}

		// save the total.
		hdr.numEntRefs = num;

		// save each area's ref list.
		for (auto& area : areas_)
		{
			stream.write(area.entRefs.ptr(), area.entRefs.size());
		}

		num = 0;

		for (uint32_t i = 0; i < MAP_MAX_MULTI_REF_LISTS; i++)
		{
			FileAreaRefHdr refHdr;
			refHdr.num = safe_static_cast<uint32_t>(multiRefEntLists_[i].size());
			refHdr.startIndex = num; // not used.

			num += refHdr.num;

			stream.write(refHdr);
		}

		// aave the tototal.
		hdr.numMultiAreaEntRefs = num;

		// write multi area ent ref lists.
		for (uint32_t i = 0; i < MAP_MAX_MULTI_REF_LISTS; i++)
		{
			stream.write(multiRefEntLists_[i].ptr(), multiRefEntLists_[i].size());
		}
	}
#endif

    // area model ent refs
    {
        auto& stream = nodeStreams[FileNodes::AREA_STATIC_MODEL_REFS];
        hdr.flags.Set(LevelFileFlags::AREA_STATIC_MODEL_REF_LISTS);

        uint32_t num = 0;

        for (auto& area : areas_) {
            FileAreaRefHdr refHdr;
            refHdr.startIndex = num;
            refHdr.num = safe_static_cast<uint32_t>(area.modelsRefs.size());

            num += safe_static_cast<uint32_t>(area.modelsRefs.size());

            stream.write(refHdr);
        }

        // save the total.
        hdr.numModelRefs = num;

        // save each area's ref list.
        for (auto& area : areas_) {
            stream.write(area.modelsRefs.ptr(), area.modelsRefs.size());
        }

        num = 0;

        for (uint32_t i = 0; i < MAP_MAX_MULTI_REF_LISTS; i++) {
            FileAreaRefHdr refHdr;
            refHdr.num = safe_static_cast<uint32_t>(multiModelRefLists_[i].size());
            refHdr.startIndex = num; // not used.

            num += refHdr.num;

            stream.write(refHdr);
        }

        // aave the tototal.
        hdr.numMultiAreaModelRefs = num;

        // write multi area ent ref lists.
        for (uint32_t i = 0; i < MAP_MAX_MULTI_REF_LISTS; i++) {
            stream.write(multiModelRefLists_[i].ptr(), multiModelRefLists_[i].size());
        }
    }

    // area collision data.
    {
        auto& stream = nodeStreams[FileNodes::AREA_COLLISION];

        hdr.flags.Set(LevelFileFlags::COLLISION);

        // we want to wrtie the collision for each area out in blocks.
        // each area can have multiple
        for (auto& area : areas_) {
            const AreaCollsiion& col = area.collision;

            AreaCollisionHdr colHdr;
            colHdr.numGroups = safe_static_cast<uint8_t>(col.numGroups());
            colHdr.trans = Vec3f::zero();

            stream.write(colHdr);

            for (const auto& group : col.getGroups()) {
                const auto& triDataArr = group.getTriMeshDataArr();
                const auto& conDataArr = group.getConvexMeshDataArr();
                const auto& aabbArr = group.getAABBData();

                static_assert(CollisionDataType::ENUM_COUNT == 4, "Enum count changed? this code may need updating");

                AreaCollisionGroupHdr groupHdr;
                groupHdr.groupFlags = group.getGroupFlags();
                core::zero_object(groupHdr.numTypes);
                groupHdr.numTypes[CollisionDataType::TriMesh] = safe_static_cast<uint16_t>(triDataArr.size());
                groupHdr.numTypes[CollisionDataType::ConvexMesh] = safe_static_cast<uint16_t>(conDataArr.size());
                groupHdr.numTypes[CollisionDataType::HeightField] = 0;
                groupHdr.numTypes[CollisionDataType::Aabb] = safe_static_cast<uint16_t>(aabbArr.size());

                stream.write(groupHdr);

                // write all the meshess.
                for (const auto& triMesh : triDataArr) {
                    const auto& cooked = triMesh.cookedData();
                    X_ASSERT(cooked.isNotEmpty(), "Collision data is empty")(); 

                    AreaCollisionDataHdr dataHdr;
                    dataHdr.dataSize = safe_static_cast<uint16_t>(cooked.size());
                    stream.write(dataHdr);
                    stream.write(cooked.data(), cooked.size());
                }

                for (const auto& convexMesh : conDataArr) {
                    const auto& cooked = convexMesh.cookedData();
                    X_ASSERT(cooked.isNotEmpty(), "Collision data is empty")(); 

                    AreaCollisionDataHdr dataHdr;
                    dataHdr.dataSize = safe_static_cast<uint16_t>(cooked.size());
                    stream.write(dataHdr);
                    stream.write(cooked.data(), cooked.size());
                }

                // write all the camels.
                stream.write(aabbArr.data(), aabbArr.size());
            }
        }
    }

    // models
    {
        auto& stream = nodeStreams[FileNodes::STATIC_MODELS];
        stream.write(staticModels_.ptr(), staticModels_.size());

        hdr.numStaticModels = safe_static_cast<int32_t>(staticModels_.size());
    }

    // bsp tree
    if (worldEnt.bspTree_.pHeadnode) {
        auto& stream = nodeStreams[FileNodes::BSP_TREE];
        hdr.flags.Set(LevelFileFlags::BSP_TREE);

        int32_t numNodes = worldEnt.bspTree_.pHeadnode->NumChildNodes();

        // set the header value.
        hdr.numNodes = numNodes;

        // need to write out all the nodes.
        // for none leaf nodes we will write the nodes number.
        // for leafs nodes we write the children as the area number but negative.
        worldEnt.bspTree_.pHeadnode->WriteNodes_r(planes_, stream);
    }

    // ents
    {
        auto& stream = nodeStreams[FileNodes::ENTITIES];

        std::array<
            core::Array<const level::LvlEntity*,
                core::ArrayAllocator<const level::LvlEntity*>,
                core::growStrat::Multiply>,
            game::ClassType::ENUM_COUNT>
            classEnts{
                X_PP_REPEAT_COMMA_SEP(7, g_arena)};

        for (size_t i = 0; i < ents.size(); i++) {
            const auto& ent = ents[i];
            classEnts[ent.classType].push_back(&ent);
        }

        JsonByteBuffer jsonBuf(stream);
        core::json::Writer<JsonByteBuffer> writer(jsonBuf);

        writer.StartObject();

        for (uint32_t i = 0; i < game::ClassType::ENUM_COUNT; i++) {
            game::ClassType::Enum type = static_cast<game::ClassType::Enum>(i);

            core::StackString256 name(game::ClassType::ToString(type));
            name.toLower();

            writer.Key(name.c_str());
            writer.StartArray();

            const auto& ents = classEnts[type];
            for (auto* pEnt : ents) {
                const auto& kvps = pEnt->epairs;

                writer.StartObject();

                for (auto it = kvps.begin(); it != kvps.end(); ++it) {
                    writer.Key(it->first, static_cast<core::json::SizeType>(it->first.length()));
                    writer.String(it->second, static_cast<core::json::SizeType>(it->second.length()));
                }

                writer.EndObject();
            }

            writer.EndArray();
        }

        writer.EndObject();

        jsonBuf.Put('\0');
    }

    // lights static
    {
        auto& stream = nodeStreams[FileNodes::LIGHTS_STATIC];

        stream.write(lights_.ptr(), lights_.size());
    }

    // lights dynamic
    {
        auto& stream = nodeStreams[FileNodes::LIGHTS_DYNAMIC];

        X_UNUSED(stream);
    }

    // update FourcCC to mark this bsp as valid.
    hdr.fourCC = LVL_FOURCC;
    hdr.numAreas = safe_static_cast<uint32_t>(areas_.size());

    for (uint32_t i = 0; i < FileNodes::ENUM_COUNT; i++) {
        auto& node = hdr.nodes[i];
        node.offset = hdr.totalDataSize;
        node.size = safe_static_cast<uint32_t>(nodeStreams[i].size());

        // all nodes are 16 byte aligned, it's required as all streams rely on fact they are 16 bytes
        // aligned to do inderpendant alignment, that is still correct once in file.
        hdr.totalDataSize += core::bitUtil::RoundUpToMultiple(node.size, 16_ui32);
    }

    hdr.datacrc32 = gEnv->pCore->GetCrc32()->GetCRC32OfObject(hdr);

    core::XFileScoped file;
    core::fileModeFlags mode;
    mode.Set(core::fileMode::WRITE);
    mode.Set(core::fileMode::RECREATE);
    mode.Set(core::fileMode::RANDOM_ACCESS); // we do a seek.

    path.setExtension(level::LVL_FILE_EXTENSION);

    if (!file.openFile(path.c_str(), mode)) {
        return false;
    }

    if (file.writeObj(hdr) != sizeof(hdr)) {
        return false;
    }

    char alignBuffer[16];
    std::memset(alignBuffer, 0x50, sizeof(alignBuffer));

    for (uint32_t i = 0; i < FileNodes::ENUM_COUNT; i++) {
        const auto& stream = nodeStreams[i];
        if (!stream.size()) {
            continue;
        }

        // write the stream, but we want to align them all.
        if (file.write(stream.begin(), stream.size()) != stream.size()) {
            return false;
        }

        const size_t fileSizeWithoutHdr = safe_static_cast<size_t>(file.tell() - sizeof(hdr));
        if (fileSizeWithoutHdr % 16 != 0) {
            size_t padSize = 16 - (fileSizeWithoutHdr % 16);
            file.writeObj(alignBuffer, padSize);
        }
    }

    return true;
}

X_NAMESPACE_END
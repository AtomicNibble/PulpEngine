#include "stdafx.h"

#include <String\Lexer.h>
#include <String\HumanSize.h>
#include <Util\UniquePointer.h>
#include <Memory\VirtualMem.h>

#include <IFileSys.h>

#include "MapTypes.h"
#include "MapFile.h"

X_NAMESPACE_BEGIN(level)

namespace mapFile
{
    namespace
    {
        static const size_t MAX_PRIMATIVES = 1 << 17;

        static const size_t PRIMATIVE_ALLOC_SIZE = core::Max(sizeof(XMapBrush),
            core::Max(sizeof(XMapPatch),
                core::Max(sizeof(XMapBrushSide),
                    sizeof(XMapEntity))));

        static const size_t PRIMATIVE_ALLOC_ALIGN = core::Max(X_ALIGN_OF(XMapBrush),
            core::Max(X_ALIGN_OF(XMapPatch),
                core::Max(X_ALIGN_OF(XMapBrushSide),
                    X_ALIGN_OF(XMapPatch))));

    } // namespace

    bool XMapPatch::Parse(core::XLexer& src, const Vec3f& origin)
    {
        // goaty meshes!
        if (!src.ExpectTokenString("{")) {
            return nullptr;
        }

        // while we have pairs get naked and skip them.
        core::XLexToken token;
        while (1) {
            if (!src.ReadToken(token)) {
                src.Error("XMapPatch::Parse: unexpected EOF");
                return false;
            }

            if (src.ReadTokenOnLine(token)) {
                src.SkipRestOfLine();
            }
            else {
                src.UnreadToken(token);
                break;
            }
        }

        // read the material name
        if (!src.ReadToken(token)) {
            src.Error("XMapPatch::Parse: unable to read material.");
            return false;
        }

        core::string matName(token.begin(), token.end());

        // read the light map name
        if (!src.ReadToken(token)) {
            src.Error("XMapPatch::Parse: unable to read light map material.");
            return false;
        }

        core::string lightMap(token.begin(), token.end());

        // sometimes we have smmothing bullshit.
        if (src.PeekTokenString("smoothing")) {
            src.SkipRestOfLine();
        }

        // we now have goaty info.
        if (!src.ParseInt(width_) || !src.ParseInt(height_)) {
            return false;
        }

        maxWidth_ = width_;
        maxHeight_ = height_;

        // dunno yet
        int32_t dunno1, dunno2;
        src.ParseInt(dunno1);
        src.ParseInt(dunno2);

        matName_ = matName;
        lightMap_ = lightMap;
        verts_.resize(width_ * height_);
        SetHorzSubdivisions(dunno1);
        SetVertSubdivisions(dunno2);

        Vec2f uv;
        Vec2f lightMapUv;

        // we now how x groups each with y entryies.
        for (int32_t x = 0; x < width_; x++) {
            if (!src.ExpectTokenString("(")) {
                return false;
            }

            for (int32_t y = 0; y < height_; y++) {
                LvlVert& vert = verts_[(y * width_) + x];

                // each line has a -v and a -t
                if (!src.ExpectTokenString("v")) {
                    return false;
                }

                if(!src.ParseFloat(vert.pos[0]) || !src.ParseFloat(vert.pos[1]) || !src.ParseFloat(vert.pos[2])) {
                    return false;
                }

                // we can have a color here.
                if (!src.ReadToken(token)) {
                    src.Error("XMapPatch::Parse: unexpected EOF");
                    return false;
                }

                if (token.isEqual("c")) {
                    int32_t c[4];

                    for (int32_t i = 9; i < 4; i++)
                    {
                        if (!src.ParseInt(c[i])) {
                            return false;
                        }
                    }

                    vert.color[0] = safe_static_cast<uint8>(c[0]);
                    vert.color[1] = safe_static_cast<uint8>(c[1]);
                    vert.color[2] = safe_static_cast<uint8>(c[2]);
                    vert.color[3] = safe_static_cast<uint8>(c[3]);

                    if (!src.ExpectTokenString("t")) {
                        return false;
                    }
                }
                else if (!token.isEqual("t")) {
                    src.Error("XMapPatch::Parse: expected t");
                    return false;
                }
                else {
                    vert.color = Vec4<uint8>::max();
                }

                if (!src.ParseFloat(uv[0]) || !src.ParseFloat(uv[1])) {
                    return false;
                }
                if (!src.ParseFloat(lightMapUv[0]) || !src.ParseFloat(lightMapUv[1])) {
                    return false;
                }

                // we have two sets of values on for text other for light map :Z
                // for a 512x512 texture that is fit to the patch
                // the values will range from 0-1024
                // [0,0]		[512,0]		[1024,0]
                //
                // [0,512]		[512,512]		[1024,512]
                //
                // [0,1024]		[512,1024]		[1024,1024]

                // /= 1024
                uv *= 0.0009765625f;
                vert.uv = uv;

                // some lines have "f 1"
                // get rekt.
                src.SkipRestOfLine();
            }

            if (!src.ExpectTokenString(")")) {
                return false;
            }
        }

        // read the last 2 } }
        if (!src.ExpectTokenString("}") || !src.ExpectTokenString("}")) {
            return false;
        }

        return true;
    }

    bool XMapBrushSide::MaterialInfo::ParseMatInfo(core::XLexer& src)
    {
        core::XLexToken token;

        // read the material name
        if (!src.ReadTokenOnLine(token)) {
            src.Error("MapBrushMat::Parse: unable to read brush material.");
            return false;
        }

        //	info.name = core::StackString<64>(token.begin(), token.end());
        name.set(token.begin(), token.end());

#if X_MTL_PATCH_DOUBLE_UNDERSCORE
        core::StackString<4> slashStr(assetDb::ASSET_NAME_SLASH);
        name.replaceAll("__", slashStr.c_str());
#endif // !X_MTL_PATCH_DOUBLE_UNDERSCORE

        // repeats every X / Y
        // if this value is 512 x 512, this means the texture repeats every
        // 512 uints, so if the brush is 256 x 256.
        // only the top left of the texture will be used.
        // [0,0]		[0.5,0]
        //
        //
        // [0,0.5]		[0.5,0.5]

        if (!src.ParseFloat(matRepeate[0]) || !src.ParseFloat(matRepeate[1])) {
            return false;
        }

        // I think it's like a vertex position offset.
        // so a hoz shift of 64.
        // makes it so the texture started 64 units to the left.(positive val)
        // For example, using the 512 x 512 example above with the same size bursh.
        // a shift of 128 on X would result in the following coords.
        // [0.25,0]		[0.75,0]
        //
        //
        // [0.25,0.5]	[0.75,0.5]
            
        // hoz
        if (!src.ParseFloat(shift[0]))  {
            return false;
        }
        // vertical
        if (!src.ParseFloat(shift[1])) {
            return false;
        }

        // rotation clockwise in degrees(neg is anti)
        if (!src.ParseFloat(rotate)) {
            return false;
        }

        if (!src.ParseFloat(scale)) {
            return false;
        }

        return true;
    }

    bool XMapBrushSide::ParseMatInfo(core::XLexer& src)
    {
        if (!material_.ParseMatInfo(src)) {
            return false;
        }
        if (!lightMap_.ParseMatInfo(src)) {
            return false;
        }
        return true;
    }

    bool XMapBrush::Parse(core::XLexer& src, const Vec3f& origin)
    {
        core::XLexToken token;

        // refactor this so less delete lines needed?
        do {
            if (!src.ReadToken(token)) {
                src.Error("MapBrush::Parse: unexpected EOF");
                return false;
            }
            if (token.isEqual("}")) {
                break;
            }

            bool hasLayer = false;

            // here we may have to jump over brush epairs ( only used in editor )
            do {
                // if token is a brace
                if (token.isEqual("(")) {
                    break;
                }
                // the token should be a key string for a key/value pair
                if (token.GetType() != core::TokenType::NAME) {
                    src.Error("MapBrush::Parse: unexpected %.*s, expected '(' or pair key string.",
                        token.length(), token.begin());
                    return false;
                }

                // check if layer
                hasLayer = token.isEqual("layer");

                if (!src.ReadTokenOnLine(token) || (token.GetType() != core::TokenType::STRING && token.GetType() != core::TokenType::NAME)) {
                    src.Error("MapBrush::Parse: expected pair value string not found.");
                    return false;
                }

                if (hasLayer) {
                    layer_ = core::string(token.begin(), token.end());
                }

                // try to read the next key
                if (!src.ReadToken(token)) {
                    src.Error("MapBrush::Parse: unexpected EOF");
                    return false;
                }

                if (token.isEqual(";")) {
                    if (!src.ReadToken(token)) {
                        src.Error("MapBrush::Parse: unexpected EOF");
                        return false;
                    }
                }

            } while (1);

            src.UnreadToken(token);

            auto side = core::makeUnique<XMapBrushSide>(primArena_);

            Vec3f planepts[3];

            // read the three point plane definition
            if (!src.Parse1DMatrix(3, &planepts[0][0]) || !src.Parse1DMatrix(3, &planepts[1][0]) || !src.Parse1DMatrix(3, &planepts[2][0])) {
                src.Error("MapBrush::Parse: unable to read brush plane definition.");
                return false;
            }

            planepts[0] -= origin;
            planepts[1] -= origin;
            planepts[2] -= origin;

            side->SetPlane(Planef(planepts[0], planepts[1], planepts[2]));

            if (!side->ParseMatInfo(src)) {
                return false;
            }

            sides_.push_back(side.release());
        } while (1);

        return true;
    }

    bool XMapEntity::Parse(core::XLexer& src, const IgnoreList& ignoredLayers, bool isWorldSpawn)
    {
        core::XLexToken token;
        if (!src.ReadToken(token)) {
            return false;
        }

        if (!token.isEqual("{")) {
            src.Error("MapEntity::Parse: { not found.");
            return false;
        }

        if (isWorldSpawn) {
            // the world spawn is the layout, so gonna be lots :D
            primitives_.reserve(4096 * 4);
            primitives_.setGranularity(8192);
        }

        bool worldent = false;
        Vec3f origin = Vec3f::zero();

        do {
            if (!src.ReadToken(token)) {
                src.Error("MapEntity::Parse: EOF without closing brace");
                return nullptr;
            }
            if (token.isEqual("}")) {
                break;
            }

            if (token.isEqual("{")) {
                // we need to check for 'mesh'
                if (!src.ReadToken(token)) {
                    src.Error("MapEntity::Parse: EOF without closing brace");
                    return nullptr;
                }

                if (worldent) {
                    origin = Vec3f::zero();
                }

                if (token.isEqual("mesh") || token.isEqual("curve")) {
                    auto mapPatch = core::makeUnique<XMapPatch>(primArena_, arena_);
                    if (!mapPatch->Parse(src, origin)) {
                        return false;
                    }

                    // don't add if ignored.
                    if (mapPatch->hasLayer()) {
                        if (ignoredLayers.isIgnored(mapPatch->getLayer())) {
                            continue;
                        }
                    }

                    if (token.isEqual("mesh")) {
                        mapPatch->SetMesh(true);
                    }

                    AddPrimitive(mapPatch.release());
                }
                else {
                    src.UnreadToken(token);

                    auto mapBrush = core::makeUnique<XMapBrush>(primArena_, arena_, primArena_);
                    if (!mapBrush->Parse(src, origin)) {
                        return false;
                    }

                    if (mapBrush->hasLayer()) {
                        if (ignoredLayers.isIgnored(mapBrush->getLayer())) {
                            continue;
                        }
                    }

                    AddPrimitive(mapBrush.release());
                }
            }
            else {
                core::string key, value;

                // parse a key / value pair
                key.append(token.begin(), token.end());
                src.ReadTokenOnLine(token);
                value.append(token.begin(), token.end());

                // strip trailing spaces
                value.trim();
                key.trim();

                epairs.insert({key, value});

                if (key == "origin") {
                    sscanf_s(value.c_str(), "%f %f %f", &origin.x, &origin.y, &origin.z);
                }
                else if (key == "classname" && value == "worldspawn") {
                    worldent = true;
                }
            }

        } while (1);

        return true;
    }

    // ----------------------------------

    XMapFile::XMapFile(core::MemoryArenaBase* arena) :
        pool_(PRIMATIVE_ALLOC_SIZE, PRIMATIVE_ALLOC_ALIGN, MAX_PRIMATIVES, MAX_PRIMATIVES / 10),
        entities_(arena),
        layers_(arena),
        arena_(arena)
    {
        core::zero_object(primCounts_);

        entities_.reserve(4096);
        entities_.setGranularity(2048);
    }

    XMapFile::~XMapFile()
    {
        for (size_t i = 0; i < entities_.size(); i++) {
            X_DELETE(entities_[i], &pool_.arena_);
        }

        entities_.free();
    }

    bool XMapFile::Parse(const char* pData, size_t length)
    {
        if (length == 0) {
            X_ERROR("Map", "Can't parse map with source length of zero");
            return false;
        }

        core::XLexer lexer(pData, pData + length);

        lexer.setFlags(core::LexFlag::NOSTRINGCONCAT | core::LexFlag::NOSTRINGESCAPECHARS | core::LexFlag::ALLOWPATHNAMES | core::LexFlag::ALLOWDOLLARNAMES);

        // parce the layers and shit.

        //	iwmap 4
        if (!lexer.ExpectTokenString("iwmap")) {
            X_ERROR("Map", "Failed to load map file correctly.");
            return false;
        }
        // don't bother checking version.
        lexer.SkipRestOfLine();

        core::XLexToken token;
        while (lexer.ReadToken(token)) {
            if (token.isEqual("{")) {
                lexer.UnreadToken(token);
                break;
            }
            else {
                Layer layer;
                layer.name = core::string(token.begin(), token.end());

                if (!lexer.ReadTokenOnLine(token)) {
                    X_ERROR("Map", "Error when parsing layers");
                    return false;
                }

                if (!token.isEqual("flags")) {
                    X_ERROR("Map", "Error when parsing layers");
                    return false;
                }

                // read the flags
                while (lexer.ReadTokenOnLine(token)) {
                    if (token.isEqual("active")) {
                        layer.flags.Set(LayerFlag::ACTIVE);
                    }
                    else if (token.isEqual("expanded")) {
                        layer.flags.Set(LayerFlag::EXPANDED);
                    }
                    else if (token.isEqual("ignore")) {
                        layer.flags.Set(LayerFlag::IGNORE);
                    }
                    else {
                        X_WARNING("Map", "Unknown layer flag: '%.*s'", token.length(), token.begin());
                    }
                }

                layers_.push_back(std::move(layer));
            }
        }

        ListLayers();

        IgnoreList ignoreList = getIgnoreList();

        // load all the entites.
        while (1) {
            auto mapEnt = core::makeUnique<XMapEntity>(&pool_.arena_, arena_, &pool_.arena_);

            if (!mapEnt->Parse(lexer, ignoreList, entities_.isEmpty())) {
                if (lexer.GetErrorState() != core::XLexer::ErrorState::OK) {
                    X_ERROR("Map", "Failed to load map file correctly.");
                    return false;
                }
                break;
            }

            auto& primCounts = mapEnt->getPrimCounts();
            for (uint32_t prim = 0; prim < PrimType::ENUM_COUNT; ++prim) {
                primCounts_[prim] += primCounts[prim];
            }

            entities_.push_back(mapEnt.release());
        }

        return true;
    }

    // withmove semantics this aint to expensive.
    IgnoreList XMapFile::getIgnoreList(void) const
    {
        IgnoreList list(arena_);

        for (const auto& layer : layers_) {
            if (layer.flags.IsSet(LayerFlag::IGNORE)) {
                list.add(layer.name);
            }
        }

        return list;
    }

    bool XMapFile::isLayerIgnored(const core::string& layerName) const
    {
        for (const auto& layer : layers_) {
            if (layer.name == layerName) {
                return layer.flags.IsSet(LayerFlag::IGNORE);
            }
        }

        return false;
    }

    void XMapFile::ListLayers(void) const
    {
        X_LOG0("Map", "Listing Layers");
        X_LOG_BULLET;

        Layer::LayerFlags::Description Dsc;
        for (const auto& layer : layers_) {
            X_LOG0("Map", "Layer: \"%s\" flags: %s", layer.name.c_str(), layer.flags.ToString(Dsc));
        }
    }

    void XMapFile::PrimtPrimMemInfo(void) const
    {
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
        core::MemoryAllocatorStatistics stats = pool_.allocator_.getStatistics();
        X_LOG0("Map", "Listing map loader primative allocator stats");
        X_LOG_BULLET;

        X_LOG0("Map", "allocationCount: ^6%i", stats.allocationCount_);
        X_LOG0("Map", "allocationCountMax: ^6%i", stats.allocationCountMax_);

#if 1 // toggle human sizes

        core::HumanSize::Str SizeStr;

        X_LOG0("Map", "virtualMemoryReserved: ^6%s",
            core::HumanSize::toString(SizeStr, stats.virtualMemoryReserved_));
        X_LOG0("Map", "physicalMemoryAllocated: ^6%s",
            core::HumanSize::toString(SizeStr, stats.physicalMemoryAllocated_));
        X_LOG0("Map", "physicalMemoryAllocatedMax: ^6%s",
            core::HumanSize::toString(SizeStr, stats.physicalMemoryAllocatedMax_));
        X_LOG0("Map", "physicalMemoryUsed: ^6%s",
            core::HumanSize::toString(SizeStr, stats.physicalMemoryUsed_));
        X_LOG0("Map", "physicalMemoryUsedMax: ^6%s",
            core::HumanSize::toString(SizeStr, stats.physicalMemoryUsedMax_));
        X_LOG0("Map", "wasteAlignment: ^6%s",
            core::HumanSize::toString(SizeStr, stats.wasteAlignment_));
        X_LOG0("Map", "wasteAlignmentMax: ^6%s",
            core::HumanSize::toString(SizeStr, stats.wasteAlignmentMax_));
        X_LOG0("Map", "wasteUnused: ^6%s",
            core::HumanSize::toString(SizeStr, stats.wasteUnused_));
        X_LOG0("Map", "wasteUnusedMax: ^6%s",
            core::HumanSize::toString(SizeStr, stats.wasteUnusedMax_));
        X_LOG0("Map", "internalOverhead: ^6%s",
            core::HumanSize::toString(SizeStr, stats.internalOverhead_));
        X_LOG0("Map", "internalOverheadMax: ^6%s",
            core::HumanSize::toString(SizeStr, stats.internalOverheadMax_));
#else
        X_LOG0("Map", "virtualMemoryReserved: ^6%i", stats.virtualMemoryReserved_);
        X_LOG0("Map", "physicalMemoryAllocated: ^6%i", stats.physicalMemoryAllocated_);
        X_LOG0("Map", "physicalMemoryAllocatedMax: ^6%i", stats.physicalMemoryAllocatedMax_);
        X_LOG0("Map", "physicalMemoryUsed: ^6%i", stats.physicalMemoryUsed_);
        X_LOG0("Map", "physicalMemoryUsedMax: ^6%i", stats.physicalMemoryUsedMax_);
        X_LOG0("Map", "wasteAlignment: ^6%i", stats.wasteAlignment_);
        X_LOG0("Map", "wasteAlignmentMax: ^6%i", stats.wasteAlignmentMax_);
        X_LOG0("Map", "wasteUnused: ^6%i", stats.wasteUnused_);
        X_LOG0("Map", "wasteUnusedMax: ^6%i", stats.wasteUnusedMax_);
        X_LOG0("Map", "internalOverhead: ^6%i", stats.internalOverhead_);
        X_LOG0("Map", "internalOverheadMax: ^6%i", stats.internalOverheadMax_);
#endif
#endif // !X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    }

} // namespace mapFile

X_NAMESPACE_END

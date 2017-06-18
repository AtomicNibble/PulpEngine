#include "stdafx.h"

#include <String\Lexer.h>
#include <String\HumanSize.h>
#include <Util\UniquePointer.h>
#include <Memory\VirtualMem.h>

#include <IFileSys.h>

#include "MapTypes.h"
#include "MapLoader.h"




X_NAMESPACE_BEGIN(mapfile)

namespace
{
	// we pool brush and patches.
	// how many as max?
	// somthing like 256k.
	// memory usage gonna be 200 * 250,000 = ~50mb o.o
	// i'll test with malloc later see what the speed diffrence is, if it's worth it.
	// or swap it for a growing pool.
	static const size_t MAX_PRIMATIVES = 1 << 17; 

	static const size_t PRIMATIVE_ALLOC_SIZE = core::Max(sizeof(XMapBrush),
		core::Max(sizeof(XMapPatch),
		core::Max(sizeof(XMapBrushSide),
		sizeof(XMapEntity))));

	
	static const size_t PRIMATIVE_ALLOC_ALIGN = core::Max(X_ALIGN_OF(XMapBrush),
		core::Max(X_ALIGN_OF(XMapPatch),
		core::Max(X_ALIGN_OF(XMapBrushSide),
		X_ALIGN_OF(XMapPatch))));

}


XMapPatch* XMapPatch::Parse(core::XLexer& src, core::MemoryArenaBase* arena, const Vec3f &origin)
{
	X_ASSERT_NOT_NULL(arena);

	// goaty meshes!
	core::XLexToken token;
	XMapPatch* patch = nullptr;

	core::StackString<level::MAP_MAX_MATERIAL_LEN> matName, lightMap;

	int width, height, dunno1, dunno2;
	int x, y;
	int c[4];

	if (!src.ExpectTokenString("{")) {
		return nullptr;
	}

	// while we have pairs get naked and skip them.
	while (1)
	{
		if (!src.ReadToken(token)) {
			src.Error("XMapPatch::Parse: unexpected EOF");
			return false;
		}

		if (src.ReadTokenOnLine(token))
		{
			src.SkipRestOfLine();
		}
		else
		{
			src.UnreadToken(token);
			break;
		}
	}

	// read the material name
	if (!src.ReadToken(token)) {
		src.Error("XMapPatch::Parse: unable to read material.");
		return false;
	}

	matName = core::StackString<level::MAP_MAX_MATERIAL_LEN>(token.begin(), token.end());

	// read the light map name
	if (!src.ReadToken(token)) {
		src.Error("XMapPatch::Parse: unable to read light map material.");
		return false;
	}

	lightMap = core::StackString<level::MAP_MAX_MATERIAL_LEN>(token.begin(), token.end());


	// sometimes we have smmothing bullshit.
	if (src.PeekTokenString("smoothing"))
	{
		src.SkipRestOfLine();
	}

	// we now have goaty info.
	width = src.ParseInt();
	height = src.ParseInt();

	// dunno yet
	dunno1 = src.ParseInt();
	dunno2 = src.ParseInt();

	patch = X_NEW(XMapPatch,arena,"MapPatch")(width, height);
	patch->matName_ = matName;
	patch->lightMap_ = lightMap;
	patch->verts_.resize(width * height);
	patch->SetHorzSubdivisions(dunno1);
	patch->SetVertSubdivisions(dunno2);

	Vec2f uv;
	Vec2f lightMapUv;

	// we now how x groups each with y entryies.
	for (x = 0; x < width; x++)
	{
		if (!src.ExpectTokenString("(")) {
			X_DELETE(patch, arena);
			return nullptr;
		}

		for (y = 0; y < height; y++)
		{
			xVert& vert = patch->verts_[(y * width) + x];

			// each line has a -v and a -t
			if (!src.ExpectTokenString("v")) {
				X_DELETE(patch, arena);
				return nullptr;
			}

			vert.pos[0] = src.ParseFloat();
			vert.pos[1] = src.ParseFloat();
			vert.pos[2] = src.ParseFloat();

			// we can have a color here.
			if (!src.ReadToken(token)) {
				src.Error("XMapPatch::Parse: unexpected EOF");
				X_DELETE(patch, arena);
				return false;
			}

			if (token.isEqual("c"))
			{
				c[0] = src.ParseInt();
				c[1] = src.ParseInt();
				c[2] = src.ParseInt();
				c[3] = src.ParseInt();

				vert.color[0] = safe_static_cast<uint8, int>(c[0]);
				vert.color[1] = safe_static_cast<uint8, int>(c[1]);
				vert.color[2] = safe_static_cast<uint8, int>(c[2]);
				vert.color[3] = safe_static_cast<uint8, int>(c[3]);

				if (!src.ExpectTokenString("t")) {
					X_DELETE(patch, arena);
					return nullptr;
				}
			}
			else if (!token.isEqual("t"))
			{
				src.Error("XMapPatch::Parse: expected t");
				X_DELETE(patch, arena);
				return false;
			}
			else
			{
				vert.color = Vec4<uint8>::max();
			}

			uv[0] = src.ParseFloat();
			uv[1] = src.ParseFloat();
			lightMapUv[0] = src.ParseFloat();
			lightMapUv[1] = src.ParseFloat();

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
			X_DELETE(patch, arena);
			return nullptr;
		}
	}

	// read the last 2 } }
	if (src.ExpectTokenString("}"))
	{
		if (src.ExpectTokenString("}"))
		{
			// valid
			return patch;
		}
	}

	X_DELETE(patch, arena);
	return nullptr;
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

	// repeats every X / Y
	// if this value is 512 x 512, this means the texture repeats every
	// 512 uints, so if the brush is 256 x 256.
	// only the top left of the texture will be used.
	// [0,0]		[0.5,0]
	//
	//
	// [0,0.5]		[0.5,0.5]
	matRepeate[0] = src.ParseFloat();
	matRepeate[1] = src.ParseFloat();

	// I think it's like a vertex position offset.
	// so a hoz shift of 64.
	// makes it so the texture started 64 units to the left.(positive val)
	// For example, using the 512 x 512 example above with the same size bursh.
	// a shift of 128 on X would result in the following coords.
	// [0.25,0]		[0.75,0]
	//
	//
	// [0.25,0.5]	[0.75,0.5]
	shift[0] = src.ParseFloat(); // hoz 
	shift[1] = src.ParseFloat(); // vertical

	// rotation clockwise in degrees(neg is anti)
	rotate = src.ParseFloat();

	// dunno what this value is, goat value?
	src.ParseFloat();
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

XMapBrush* XMapBrush::Parse(core::XLexer& src, core::MemoryArenaBase* arena, const Vec3f& origin)
{
	core::XLexToken token;

	auto brush = core::makeUnique<XMapBrush>(arena);

	// refactor this so less delete lines needed?
	do
	{
		if (!src.ReadToken(token)) {
			src.Error("MapBrush::Parse: unexpected EOF");
			return nullptr;
		}
		if (token.isEqual("}")) {
			break;
		}

		bool hasLayer = false;

		// here we may have to jump over brush epairs ( only used in editor )
		do
		{
			// if token is a brace
			if (token.isEqual("(")) {
				break;
			}
			// the token should be a key string for a key/value pair
			if (token.GetType() != core::TokenType::NAME) {
				src.Error("MapBrush::Parse: unexpected %.*s, expected '(' or pair key string.",
					token.length(), token.begin());
				return nullptr;
			}

			// check if layer
			hasLayer = token.isEqual("layer");

			if (!src.ReadTokenOnLine(token) || (token.GetType() != core::TokenType::STRING
				&& token.GetType() != core::TokenType::NAME))
			{
				src.Error("MapBrush::Parse: expected pair value string not found.");
				return nullptr;
			}

			if (hasLayer) {			
				brush->layer_ = core::string(token.begin(), token.end());
			}

			// try to read the next key
			if (!src.ReadToken(token)) {
				src.Error("MapBrush::Parse: unexpected EOF");
				return nullptr;
			}

			if (token.isEqual(";")) {
				if (!src.ReadToken(token)) {
					src.Error("MapBrush::Parse: unexpected EOF");
					return nullptr;
				}
			}

		} while (1);

		src.UnreadToken(token);

		auto side = core::makeUnique<XMapBrushSide>(arena);

		Vec3f planepts[3];

		// read the three point plane definition
		if (!src.Parse1DMatrix(3, &planepts[0][0]) ||
			!src.Parse1DMatrix(3, &planepts[1][0]) ||
			!src.Parse1DMatrix(3, &planepts[2][0])) {
			src.Error("MapBrush::Parse: unable to read brush plane definition.");
			return nullptr;
		}

		planepts[0] -= origin;
		planepts[1] -= origin;
		planepts[2] -= origin;

		side->plane_.set(planepts[0], planepts[1], planepts[2]);

		if (!side->ParseMatInfo(src)) {
			return nullptr;
		}

		brush->sides.push_back(side.release());
	} while (1);

	return brush.release();
}


XMapEntity*	XMapEntity::Parse(core::XLexer& src, core::MemoryArenaBase* arena,
	const IgnoreList& ignoredLayers, bool isWorldSpawn)
{
	core::XLexToken token;
	XMapEntity* mapEnt;
	Vec3f origin;
	float v1, v2, v3;
	bool worldent;

	if (!src.ReadToken(token)) {
		return nullptr;
	}

	if (!token.isEqual("{")) {
		src.Error("MapEntity::Parse: { not found.");
		return nullptr;
	}

	mapEnt = X_NEW(XMapEntity, arena, "MapEntity");
	mapEnt-> primArena_ = arena;

	if (isWorldSpawn) {
		// the world spawn is the layout, so gonna be lots :D
		mapEnt->primitives.reserve(4096 * 8);
		mapEnt->primitives.setGranularity(4096);
	}

	worldent = false;
	origin = Vec3f::zero();

	do
	{
		if (!src.ReadToken(token)) {
			src.Error("MapEntity::Parse: EOF without closing brace");
			return nullptr;
		}
		if (token.isEqual("}")) {
			break;
		}

		if (token.isEqual("{"))
		{
			// we need to check for 'mesh'
			if (!src.ReadToken(token)) {
				src.Error("MapEntity::Parse: EOF without closing brace");
				return nullptr;
			}

			if (worldent) {
				origin = Vec3f::zero();
			}

			if (token.isEqual("mesh") || token.isEqual("curve"))
			{
				XMapPatch* mapPatch = XMapPatch::Parse(src, arena, origin);
				if (!mapPatch) {
					return nullptr;
				}
				// don't add if ignored.
				if (mapPatch->hasLayer()) {
					if (ignoredLayers.isIgnored(mapPatch->getLayer())) {
						X_DELETE(mapPatch, arena);
						continue;
					}
				}

				if (token.isEqual("mesh")) {
					mapPatch->SetMesh(true);
				}

				mapEnt->AddPrimitive(mapPatch);
			}
			else
			{
				src.UnreadToken(token);
				XMapBrush* mapBrush = XMapBrush::Parse(src, arena, origin);
				if (!mapBrush) {
					return nullptr;
				}
				// don't add if ignored.
				if (mapBrush->hasLayer()) {
					if (ignoredLayers.isIgnored(mapBrush->getLayer())) {
						X_DELETE(mapBrush, arena);
						continue;
					}
				}

				mapEnt->AddPrimitive(mapBrush);
			}

		}
		else
		{
			core::StackString512 key, value;

			// parse a key / value pair
			key.append(token.begin(), token.end());
			src.ReadTokenOnLine(token);
			value.append(token.begin(), token.end());

			// strip trailing spaces
			value.trim();
			key.trim();

			mapEnt->epairs[core::string(key.c_str())] = value.c_str();

			if (key.isEqual("origin"))
			{
				v1 = v2 = v3 = 0;
				sscanf_s(value.c_str(), "%f %f %f", &v1, &v2, &v3);
				origin.x = v1;
				origin.y = v2;
				origin.z = v3;
			}
			else if (key.isEqual("classname") && value.isEqual("worldspawn")) {
				worldent = true;
			}
		}

	} while (1);

	return mapEnt;
}

// ----------------------------------

XMapFile::XMapFile() :
#if MAP_LOADER_USE_POOL
primPoolAllocator_(
	core::bitUtil::NextPowerOfTwo(
		PrimativePoolArena::getMemoryRequirement(PRIMATIVE_ALLOC_SIZE) * MAX_PRIMATIVES
	),
	core::bitUtil::NextPowerOfTwo(
		PrimativePoolArena::getMemoryRequirement(PRIMATIVE_ALLOC_SIZE) * (MAX_PRIMATIVES / 10)
	),
	0,
	PrimativePoolArena::getMemoryRequirement(PRIMATIVE_ALLOC_SIZE),
	PrimativePoolArena::getMemoryAlignmentRequirement(PRIMATIVE_ALLOC_ALIGN),
	PrimativePoolArena::getMemoryOffsetRequirement()
),
primPoolArena_(&primPoolAllocator_, "PrimativePool"),
#else
primPoolArena_(&primAllocator_, "PrimativePool"),
#endif

entities_(g_arena),
layers_(g_arena)
{
	numBrushes_ = 0;
	numPatches_ = 0;

	// we typically will have a few hundred models, a load of triggers etc.
	// might make this 8k
	// worldspawn is 1 entity.
	entities_.reserve(4096 * 2);
	entities_.setGranularity(512);
}

XMapFile::~XMapFile()
{
	EntityArray::size_type i;
	for (i = 0; i < entities_.size(); i++)
	{
		X_DELETE(entities_[i], &primPoolArena_);
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
	core::XLexToken token;
	XMapEntity *mapEnt;

	lexer.setFlags(core::LexFlag::NOSTRINGCONCAT |
		core::LexFlag::NOSTRINGESCAPECHARS |
		core::LexFlag::ALLOWPATHNAMES |
		core::LexFlag::ALLOWDOLLARNAMES);

	// parce the layers and shit.

	//	iwmap 4 
	if (!lexer.ExpectTokenString("iwmap")) {
		X_ERROR("Map", "Failed to load map file correctly.");
		return false;
	}
	// don't bother checking version.
	lexer.SkipRestOfLine();

	while (lexer.ReadToken(token))
	{
		if (token.isEqual("{"))
		{
			lexer.UnreadToken(token);
			break;
		}
		else
		{
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
			while (lexer.ReadTokenOnLine(token))
			{
				core::string flag(token.begin(), token.end());

				if (flag.compare("active")) {
					layer.flags.Set(LayerFlag::ACTIVE);
				}
				else if (flag.compare("expanded")) {
					layer.flags.Set(LayerFlag::EXPANDED);
				}
				else if (flag.compare("ignore")) {
					layer.flags.Set(LayerFlag::IGNORE);
				}
				else {
					X_WARNING("Map", "Unknown layer flag: '%s'",
						flag.c_str());
				}
			}

			layers_.push_back(layer);

		}
	}

	ListLayers();

	IgnoreList ignoreList = getIgnoreList();

	// load all the entites.
	while (1)
	{
		mapEnt = XMapEntity::Parse(lexer, &primPoolArena_, ignoreList, entities_.isEmpty());

		if (!mapEnt)
		{
			if (lexer.GetErrorState() != core::XLexer::ErrorState::OK) {
				X_ERROR("Map", "Failed to load map file correctly.");
				return false;
			}
			break;
		}

		for (size_t i = 0; i < mapEnt->GetNumPrimitives(); i++)
		{
			const XMapPrimitive* prim = mapEnt->GetPrimitive(i);

			if (prim->getType() == PrimType::BRUSH) {
				numBrushes_++;
			}
			else if (prim->getType() == PrimType::PATCH) {
				numPatches_++;
			}
		}

		entities_.push_back(mapEnt);
	}


	PrimtPrimMemInfo();
	return true;
}

// withmove semantics this aint to expensive.
IgnoreList XMapFile::getIgnoreList(void) const
{
	core::Array<core::string> list(g_arena);

	for (const auto& layer : layers_)
	{
		if (layer.flags.IsSet(LayerFlag::IGNORE)) {
			list.append(layer.name);
		}
	}

	return IgnoreList(std::move(list));
}

bool XMapFile::isLayerIgnored(const core::string& layerName) const
{
	for (const auto& layer : layers_)
	{
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
	for (const auto& layer : layers_)
	{
		X_LOG0("Map", "Layer: \"%s\" flags: %s", layer.name.c_str(), layer.flags.ToString(Dsc));
	}
}


void XMapFile::PrimtPrimMemInfo(void) const
{
#if MAP_LOADER_USE_POOL && X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	core::MemoryAllocatorStatistics stats = primPoolAllocator_.getStatistics();
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
#endif // !MAP_LOADER_USE_POOL && X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
}



X_NAMESPACE_END

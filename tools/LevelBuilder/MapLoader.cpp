#include "stdafx.h"

#include <String\Lexer.h>
#include <String\StackString.h>

#include <IFileSys.h>

#include "MapTypes.h"
#include "MapLoader.h"


#include <Memory\VirtualMem.h>


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

	static const size_t PRIMATIVE_ALLOC_SIZE = core::Max_static_size<sizeof(XMapBrush),
		core::Max_static_size<sizeof(XMapPatch), 
		core::Max_static_size<sizeof(XMapBrushSide), 
		sizeof(XMapEntity)>::value>::value>::value;

	
	static const size_t PRIMATIVE_ALLOC_ALIGN = core::Max_static_size<X_ALIGN_OF(XMapBrush),
		core::Max_static_size<X_ALIGN_OF(XMapPatch),
		core::Max_static_size<X_ALIGN_OF(XMapBrushSide),
		X_ALIGN_OF(XMapPatch)>::value>::value>::value;

}


XMapPatch* XMapPatch::Parse(XLexer &src, core::MemoryArenaBase* arena, const Vec3f &origin)
{
	X_ASSERT_NOT_NULL(arena);

	// goaty meshes!
	XLexToken token;
	XMapPatch* patch = nullptr;

	core::StackString<level::MAP_MAX_MATERIAL_LEN> matName, lightMap;

	int width, height, dunno1, dunno2;
	int x, y;
	float t[4];
	int  c[4];

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

			t[0] = src.ParseFloat();
			t[1] = src.ParseFloat();
			t[2] = src.ParseFloat();
			t[3] = src.ParseFloat();

			// 4 tex cords what is this shit!
			vert.uv[0] = t[0];
			vert.uv[1] = t[1];

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



bool XMapBrushSide::ParseMatInfo(XLexer &src, XMapBrushSide::MaterialInfo& info)
{
	XLexToken token;

	// read the material name
	if (!src.ReadTokenOnLine(token)) {
		src.Error("MapBrushMat::Parse: unable to read brush material.");
		return false;
	}

//	info.name = core::StackString<64>(token.begin(), token.end());
	info.name.set(token.begin(), token.end());

	// repeats every X / Y
	// if this value is 512 x 512, this means the texture repeats every
	// 512 uints, so if the brush is 256 x 256.
	// only the top left of the texture will be used.
	// [0,0]		[0.5,0]
	//
	//
	// [0,0.5]		[0.5,0.5]
	info.matRepeate[0] = src.ParseFloat();
	info.matRepeate[1] = src.ParseFloat();

	// I think it's like a vertex position offset.
	// so a hoz shift of 64.
	// makes it so the texture started 64 units to the left.(positive val)
	// For example, using the 512 x 512 example above with the same size bursh.
	// a shift of 128 on X would result in the following coords.
	// [0.25,0]		[0.75,0]
	//
	//
	// [0.25,0.5]	[0.75,0.5]
	info.shift[0] = src.ParseFloat(); // hoz 
	info.shift[1] = src.ParseFloat(); // vertical

	// rotation clockwise in degrees(neg is anti)
	info.rotate = src.ParseFloat();

	// dunno what this value is, goat value?
	src.ParseFloat();
	return true;
}

XMapBrush* XMapBrush::Parse(XLexer& src, core::MemoryArenaBase* arena, const Vec3f& origin)
{
	X_ASSERT_NOT_NULL(arena);

	Vec3f planepts[3];
	XLexToken token;
	XMapBrushSide* side;
	XMapBrush* brush;

	brush = X_NEW( XMapBrush, arena, "MapBrush");

	// refactor this so less delete lines needed?
	do
	{
		if (!src.ReadToken(token)) {
			src.Error("MapBrush::Parse: unexpected EOF");
			X_DELETE(brush, arena);
			return nullptr;
		}
		if (token.isEqual("}")) {
			break;
		}

		// here we may have to jump over brush epairs ( only used in editor )
		do
		{
			// if token is a brace
			if (token.isEqual("(")) {
				break;
			}
			// the token should be a key string for a key/value pair
			if (token.GetType() != TokenType::NAME) {
				src.Error("MapBrush::Parse: unexpected %.*s, expected '(' or pair key string.",
					token.length(), token.begin());
				X_DELETE(brush, arena);
				return nullptr;
			}

			if (!src.ReadTokenOnLine(token) || (token.GetType() != TokenType::STRING
				&& token.GetType() != TokenType::NAME))
			{
				src.Error("MapBrush::Parse: expected pair value string not found.");
				X_DELETE(brush, arena);
				return nullptr;
			}

			// try to read the next key
			if (!src.ReadToken(token)) {
				src.Error("MapBrush::Parse: unexpected EOF");
				X_DELETE(brush, arena);
				return nullptr;
			}

			if (token.isEqual(";")) {
				if (!src.ReadToken(token)) {
					src.Error("MapBrush::Parse: unexpected EOF");
					X_DELETE(brush, arena);
					return nullptr;
				}
			}

		} while (1);

		src.UnreadToken(token);

		side = X_NEW( XMapBrushSide, arena, "MapBrushSide");
		brush->sides.push_back(side);

		// read the three point plane definition
		if (!src.Parse1DMatrix(3, &planepts[0][0]) ||
			!src.Parse1DMatrix(3, &planepts[1][0]) ||
			!src.Parse1DMatrix(3, &planepts[2][0])) {
			src.Error("MapBrush::Parse: unable to read brush plane definition.");
			X_DELETE(brush, arena);
			return nullptr;
		}

		planepts[0] -= origin;
		planepts[1] -= origin;
		planepts[2] -= origin;

		side->plane.set(planepts[0], planepts[1], planepts[2]);

		XMapBrushSide::ParseMatInfo(src, side->material);
		XMapBrushSide::ParseMatInfo(src, side->lightMap);

	} while (1);


	return brush;
}


XMapEntity*	XMapEntity::Parse(XLexer& src, core::MemoryArenaBase* arena, bool isWorldSpawn)
{
	XLexToken token;
	XMapEntity *mapEnt;
	XMapBrush *mapBrush;
	XMapPatch *mapPatch;
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
				mapPatch = XMapPatch::Parse(src, arena, origin);
				if (!mapPatch) {
					return nullptr;
				}

				if (token.isEqual("mesh")) {
					mapPatch->SetMesh(true);
				}

				mapEnt->AddPrimitive(mapPatch);
			}
			else
			{
				src.UnreadToken(token);
				mapBrush = XMapBrush::Parse(src, arena, origin);
				if (!mapBrush) {
					return nullptr;
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
			//	value.trimWhitespace();
			//	key.trimWhitespace();

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
primPoolHeap_(
	bitUtil::RoundUpToMultiple<size_t>(
		PrimativePoolArena::getMemoryRequirement(PRIMATIVE_ALLOC_SIZE) * MAX_PRIMATIVES,
		core::VirtualMem::GetPageSize()
	)
),
primPoolAllocator_(primPoolHeap_.start(), primPoolHeap_.end(),
	PrimativePoolArena::getMemoryRequirement(PRIMATIVE_ALLOC_SIZE),
	PrimativePoolArena::getMemoryAlignmentRequirement(PRIMATIVE_ALLOC_ALIGN),
	PrimativePoolArena::getMemoryOffsetRequirement()
),
primPoolArena_(&primPoolAllocator_, "PrimativePool"),
#else
primPoolArena_(&primAllocator_, "PrimativePool"),
#endif

entities_(g_arena)
{
	numBrushes = 0;
	numPatches = 0;

	// we typically will have a few hundred models, a load of triggers etc.
	// might make this 8k
	// worldspawn is 1 entity.
	entities_.reserve(4096 * 2);
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
	int i = 0;
	if (length > 0)
	{
		XLexer lexer(pData, pData + length);
		XLexToken token;
		XMapEntity *mapEnt;

		lexer.setFlags(LexFlag::NOSTRINGCONCAT | LexFlag::NOSTRINGESCAPECHARS | LexFlag::ALLOWPATHNAMES);

		// we need to parse up untill the first brace.
		while (lexer.ReadToken(token))
		{
			if (token.isEqual("{"))
			{
				lexer.UnreadToken(token);
				break;
			}
		}

		// load all the entites.
		while (1) 
		{
			mapEnt = XMapEntity::Parse(lexer, &primPoolArena_, entities_.isEmpty());
			if (!mapEnt) 
			{
				if (lexer.GetErrorState() != XLexer::ErrorState::OK) {
					X_ERROR("Map", "Failed to load map file correctly.");
					return false;
				}
				break;
			}

			for (i = 0; i < mapEnt->GetNumPrimitives(); i++)
			{
				const XMapPrimitive* prim = mapEnt->GetPrimitive(i);

				if (prim->getType() == PrimType::BRUSH) {
					this->numBrushes++;
				}
				else if (prim->getType() == PrimType::PATCH) {
					this->numPatches++;
				}
			}

			entities_.push_back(mapEnt);
		}
	}

	return true;
}


X_NAMESPACE_END

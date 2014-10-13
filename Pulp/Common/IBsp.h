#pragma once

#ifndef X_BSP_MAP_H_
#define X_BSP_MAP_H_

#include <Math\VertexFormats.h>
#include <String\StackString.h>
#include <Time\CompressedStamps.h>

X_NAMESPACE_BEGIN(bsp)


//
// Some Refrences: (in no order)
//	* These refrences very useful, gave me a good understanding of
//		how quake bsp are made, and the goals + optermisations.
//		which has allowed me to develop my own version.
//
//  http://www.mralligator.com/q3/
//  http://www.flipcode.com/archives/Quake_2_BSP_File_Format.shtml
//  http://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_4.htm
//  http://en.wikipedia.org/wiki/BSP_%28file_format%29
//  https://developer.valvesoftware.com/wiki/Source_BSP_File_Format
//  
//	http://fabiensanglard.net/doom3/dmap.php
//	http://www.mralligator.com/q3/#Faces
//
// File Layout:
//		||||| Header ||||||
//		||||| BlockHeader[ numblocks ] |||||| <- block layout from quake bsp, good idear makes file quite flexible can omit blocks etc.
//      ||||| blockData[ numblocks ] ||||	
//
//
//	Info: (talking to myself / planning stuff)
//	
//	we take a .map file which contains multiple models.
//	the first been the world models.
//
//	each model contains 1.* entites which can be a brush / patch.
//
//	we take all the brushs and patches and turn it into a vertex / index list.
//	so that we can draw the whole level.
//
//	Optimisation 1: Duplicates
//
//		We try and share verts and indexes as much as possible in order
//		to reduce the size of the vertex / index buffers.
//
//	Optimisation 2: Portals
//
//		The map's will use portals to split up the map in to area's	
//		This means that each area may or not be drawn.
//		and stuff in the area should be grouped and sorted.
//		like a mini batch.
//
//		Instead of global grouping.
//
//		Should I split the vertex / index lists into seprate blocks?
//		This would allow only parts of the map to be on the gpu.
//		
//		I don't really see the point, since I'm not going to be supporting huge 
//		levels		
//		
//		And the size of the maps whole vertex buffer is going to be nothing
//		compare to the models, 5-6 high poly players models probs be more verts lol.
//
//		so.. Duplicates will be check on a level basis not area!
//
//		--------------------------------------
//
//		I will calculate vis info for the area's so that i know what
//		area's are visable when inside a area.
//
//		probs use my intrusive lists to link them, since it supports a item
//		been in multiple lists, then i just work out which area the player is in.
//		and draw the list.
//
//	Optimisation 3: Static Lights / Shadows
//
//	There will be some lights that are baked in.
//	ones that never change color or intensity.
//
//	Probs not that many though since most lights will be dynamic
//	With the ability to turn on / off so that I can a rooms lights on / off on the fly!
//
//	not sure how much I can / make static yet have to see later.
//
//	Optimisation 4: group drawing
//
//	If a area has multiple surfaces with the same surface type and shader.
//	which is super fucking likley.
//	
//	I should group that into a single draw.
//
//	Should I just make it a single surface? don't see why not.
//	
//	Depends if i have other uses for the surface info other than drawing.
//
//	================== Lump Info =========================
//
//	Info about what's in each lump.
//
//	Entities: 
//	
//		..
//
//	Materials:
//		All the materials used by the level.
//		collection of struct BSPMaterial;
//
//		Surfaces store the index of the material that it uses.
//		This means no duplicates and the order is important.
//
//	Planes:
//	
//
//
//	Verts:
//		Array of BSP::Vertex.
//
//	Indexes:
//		Array of BSP::Index currently 4 bytes, allows more than 1 << 16 verts
//		and it's only sent to gpu once, so only issue is size it uses and 
//		gpu has to move more data.
//
//		Mappers be moaning if it's max 65k ^^ 
//
//	BrushSide:
//		Array of BSP::BrushSide, gives the brushes side plane index.
//		So that the plane for this side can be looked up in the plane list.
//
//	Brush:
//		Array of BSP::Brush, provides the starting index in BrushSide lump,
//		and the number of sides, so all the side info can be got with.
//
//		BrushSide[offset + i] where i < numSides;
//
//	Surfaces:
//		Array of BSP::Surface, info for drawing each surface.
//
//	Areas:
//		Areas are basically a section of the map, or the whole map (in the case of zero portals)
//		Each area is seperated by one or more portals.
//
//		It provides surface info offset, so that all the surfaces for this area can be located.
//
//	=== Rendering ===
//
//	How to render this sexy pickle.
//	Each area is a model basically
//	with multiple sub meshes for all the diffrent 
//	faces.
//	
//	All faces in area with same texture should be rendered as a single call.
//
//	I want a single area to be a single VB / IB, or dose it matter?
//	would save rebinding for each area(if i use single VB / IB for whole map.)
//
//	for reloading the map seprate area vb's would help i guess.
//	it makes a diffrence for the BSP compiler as i would check for duplicates
//	across whole level, so i can't make it a engine option. (well i could but not have all benfits)
//
//	we have a collection of surfaces for the map.
//	they are grouped for each area
//	so that each area has a start index and draws the surfaces for that area.
//
//	=== Culling ===
//
//	Even tho I am only going to be rendering area's i may want to now render the
//	whole area. so I will check onjects in the area to see if they should be culled.
//	
//	This is fine since even tho everything in the area will be in a single VB / IB
//	and each one is draw seprate, but we have the problem of gouping.
//
//	since I group everything that uses same material etc.
//	It means that if you can see some object in that area all others that use it will be drawn
//	Thinking about it, this won't really be a issue since the map's structure is quite low
//	poly, since models are seprate.
//
//	And a map won't typically have too much texture reusesage for the slightly large number of 
//	planes be drawn to be a issue.
//

static const uint32_t	 BSP_VERSION = 2; //  chnage everytime the format changes.
static const uint32_t	 BSP_FOURCC = X_TAG('x', 'b', 's', 'p');
static const uint32_t	 BSP_FOURCC_INVALID = X_TAG('x', 'e', 'r', 'r'); // if a file falid to write the final header, this will be it's FourCC
static const char*		 BSP_FILE_EXTENSION = ".xbsp";

// a level can not exceed this size.
static const int32_t	 MAX_WORLD_COORD = (128 * 1024);
static const int32_t	 MIN_WORLD_COORD = (-128 * 1024);
static const int32_t	 MAX_WORLD_SIZE = (MAX_WORLD_COORD - MIN_WORLD_COORD);

// All these limits are applied AFTER it has been compiled.
// as a map will typically have less sides etc once compiled.
// I currently check these when loading aswell as compiling.
// Limits must be obeyed ;)
static const uint32_t	 MAP_MAX_PLANES = 65536;
static const uint32_t	 MAP_MAX_VERTS = 65536;
static const uint32_t	 MAP_MAX_INDEXES = 65536;
static const uint32_t	 MAP_MAX_BRUSHES = 32768;
static const uint32_t	 MAP_MAX_EDGES = (65536) * 4;
static const uint32_t	 MAP_MAX_ENTITIES = 16384;
static const uint32_t	 MAP_MAX_BRUSHSIDES = 65536;	// total sides in map / bsp
static const uint32_t	 MAP_MAX_SIDES_PER_BRUSH = 64;	// max sides a single brush can have.
static const uint32_t	 MAP_MAX_LEAFS = 65536;

static const uint32_t	 MAP_MAX_MODELS = 1024;
static const uint32_t	 MAP_MAX_LIGHTS_WORLD = 4096;
static const uint32_t	 MAP_MAX_NODES = 2048;
static const uint32_t	 MAP_MAX_TEXTURES = 1024;


static const uint32_t	 MAP_MAX_MATERIAL_LEN = 64;


// Key / Value limits
static const uint32_t	 MAX_KEY_LENGTH = 64;			// KVP: name
static const uint32_t	 MAX_VALUE_LENGTH = 256;		// KVP: value


X_DECLARE_FLAGS(MatContentFlags)(SOLID, WATER, PLAYER_CLIP, MONSTER_CLIP, TRIGGER, NO_FALL_DMG, DETAIL, STRUCTURAL, ORIGIN);
X_DECLARE_FLAGS(MatSurfaceFlags)(NO_DRAW, LADDER);

// may add more as i make them.
X_DECLARE_ENUM(LumpType)(Entities, Materials, Planes, Verts, Indexes, BrushSide, Brush, Surfaces, Areas);
X_DECLARE_ENUM(SurfaceType)(Invalid,Plane, Patch);


typedef Flags<MatContentFlags> MatContentFlag;
typedef Flags<MatSurfaceFlags> MatSurfaceFlag;

struct Material
{
	core::StackString<MAP_MAX_MATERIAL_LEN> Name;
	MatSurfaceFlag		surfaceFlag;
	MatContentFlag		contentFlag;
};


typedef Vertex_P3F_T4F_N3F_C4B Vertex;

typedef int Index;


struct Surface
{
	int32_t				materialIdx;
	SurfaceType::Enum	surfaceType; // 8

	int32_t				vertexStartIdx;
	int32_t				numVerts;	// 16

	int32_t				indexStartIdx;
	int32_t				numIndexes;	// 24

	Vec3f				normal;		// 36
	Vec2<int32_t>		patchSize;	// 44
}; // 44

struct BrushSide
{
	int32_t plane;
	int32_t shaderNum;
};

struct Brush
{
	int32_t	firstSide;
	int32_t	numSides;
	int32_t	shaderNum;
};

struct Area
{
	int32_t surfaceStartIdx;
	int32_t numSurfaces;

	int32_t brushStartIds;
	int32_t numBrushes;

	AABB bounds;
};

// gives offset to the lump.
struct FileLump
{
	uint32_t offset;
	uint32_t size;

	const bool isValid(void) const {
		return offset > 0 && size > 0;
	}
};


struct FileHeader
{
	uint32_t fourCC;
	uint8_t  version;
	uint8_t  blank[3];

	core::dateTimeStampSmall modified;

	// crc32 is made from just the lump info.
	// used for reload checks.
	// this might make a pretty good integrity check.
	// as what 4 bytes are they gonna change without fucking up the file.
	// unless they change last lumps offset to fix crc32 and null pad the shit out
	// of the file lol.
	uint32_t datacrc32;

	FileLump lumps[LumpType::ENUM_COUNT];

	const bool isValid(void) const {
		return fourCC == BSP_FOURCC;
	}
};



X_ENSURE_SIZE(Material, 76);
// X_ENSURE_SIZE(BSPNode, 36);
// X_ENSURE_SIZE(BSPLeaf, 48);
// X_ENSURE_SIZE(BSPModel, 40);

X_ENSURE_SIZE(Vertex, 44);
X_ENSURE_SIZE(Surface, 44);

X_ENSURE_SIZE(BrushSide, 8);
X_ENSURE_SIZE(Brush, 12);

X_ENSURE_SIZE(FileLump, 8);

X_ENSURE_SIZE(FileHeader, (16 + (8 * LumpType::ENUM_COUNT)));

X_NAMESPACE_END

#endif // !X_BSP_MAP_H_
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
//		||||| BlockHeader[ numblocks ] ||||||
//      ||||| blockData[ numblocks ] ||||	
//
//	This layout is simular to quake bsp's I more than likly will update this layout
//	to one i can map in memory so i don't have to allocate other buffers.
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
//		Used for stuff like finding where the camera is.
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
//  Nodes:
//		Used to find the leaf we are in.
//		We traverse it untill we find a leaf.
//
//  Leafs:
//		A leaf has info like bounds and cluster.
//		If we get the Leaf from traversing the nodes.
//
//		We can use that cluster to check what other leaves are potentialy visable.
//		By doing a bit lookup in the VisData
//
//	Areas:
//		Areas are basically a section of the map, or the whole map (in the case of zero portals)
//		Each area is seperated by one or more portals.
//
//		It provides surface info offset, so that all the surfaces for this area can be located.
//
//	Portals:
//
//		
//
//
//	=== Rendering ===
//
//	Step 1:
//		
//		Find out what are the camera is in.
//		This is done by traversing the BSP tree from the root.
//		checking what side of the plane we are on.
//		
//		Once we have reached a leaf node, we can get the area number from the leaf.
//
//	Step 2:	
//		
//		Now that we have the area, we want to know what other area's needed to be rendered.
//		Each area has a collection of portals, for each portal we check that one the plane of
//		the portals plane is facing away from us. (aka the direction of the portal, leaving the area basically).
//		
//		We then clip out current portal planes with the winding of the portal.
//		If the clipped area is zero in size then it's not visable.
//		
//		When a portal is visable, we then go into the next area, passing the current clipped planes
//		Down via a stack so that the next area's portal must then be inside the current portal.
//		
//	Step 3:
//
//		We can then use this info to create scissor rects, so that when we draw the area visable via
//		the doorway or what ever, only the shit inside the doorway will have pixel operations run.
//
//		The scissor is a retangle, so the smallest rectange that contains the portal is made.
//		Meaning some overdraw will occur but very minimal.
//
//	Step 4:	
//
//		Each area that has been determined as visable, is then drawn.
//		Optionaly do I want to frustrum cull geo?
//		Or keep that to just FX's
//
//	-----------------------------------------
//
//	Each area in the map is considered to be a model with x surfaces.
//	
//
//

static const uint32_t	 BSP_VERSION = 4; //  chnage everytime the format changes. (i'll reset it once i'm doing messing around)
static const uint32_t	 BSP_FOURCC = X_TAG('x', 'l', 'v', 'l');
static const uint32_t	 BSP_FOURCC_INVALID = X_TAG('x', 'e', 'r', 'r'); // if a file falid to write the final header, this will be it's FourCC
// feels kinda wrong to call it a '.bsp', since it's otherthings as well. 
// '.level' is more pleasing to me and more importantly the BushMaster of Christmas Island(Southeast Asia).
static const char*		 BSP_FILE_EXTENSION = ".level"; // ".bsp";

// a level can not exceed this size.
static const int32_t	 MAX_WORLD_COORD = (128 * 1024);
static const int32_t	 MIN_WORLD_COORD = (-128 * 1024);
static const int32_t	 MAX_WORLD_SIZE = (MAX_WORLD_COORD - MIN_WORLD_COORD);

// some of these limts are done on .map load.
// others checked while compiling bsp.
// slap a hat, swing a rat and hit it with a bat.
static const uint32_t	 MAP_MAX_PLANES = 65536;		
static const uint32_t	 MAP_MAX_VERTS = 65536;			
static const uint32_t	 MAP_MAX_INDEXES = 65536;		
static const uint32_t	 MAP_MAX_BRUSHES = 32768;		
static const uint32_t	 MAP_MAX_BRUSHSIDES = 65536;		// total sides in map / bsp
static const uint32_t	 MAP_MAX_SIDES_PER_BRUSH = 64;		// max sides a single brush can have.
static const uint32_t	 MAP_MAX_NODES = 65536;
static const uint32_t	 MAP_MAX_LEAFS = 65536;
static const uint32_t	 MAP_MAX_AREAS = 0x100;
static const uint32_t	 MAP_MAX_PORTALS = 0x100;
static const uint32_t	 MAP_MAX_SURFACES = 65536;		
static const uint32_t	 MAP_MAX_MODELS = 0x400;			// a model is a 'area'.
static const uint32_t	 MAP_MAX_MODEL_SURFACES = 65536;	// the maximum surfaces a map model can have.
static const uint32_t	 MAP_MAX_MATERIALs = 0x800;		

// Should be checked in compiler.
static const uint32_t	 MAP_MAX_ENTITES = 0x400;
static const uint32_t	 MAP_MAX_LIGHTS_WORLD = 4096;

// might be removed in-favor of embeded binary materials
static const uint32_t	 MAP_MAX_MATERIAL_LEN = 64;

// Key / Value limits
static const uint32_t	 MAX_KEY_LENGTH = 64;			// KVP: name
static const uint32_t	 MAX_VALUE_LENGTH = 256;		// KVP: value

// show me the light, o holy one.
static const uint32_t	 LIGHT_MAP_WIDTH = 128;
static const uint32_t	 LIGHT_MAP_HEIGHT = 128;


// forward Decs.
class XWinding;
// ~forward Decs.


X_DECLARE_FLAGS(MatContentFlags)(SOLID, WATER, PLAYER_CLIP, MONSTER_CLIP, TRIGGER, NO_FALL_DMG, DETAIL, STRUCTURAL, ORIGIN);
X_DECLARE_FLAGS(MatSurfaceFlags)(NO_DRAW, LADDER);

// may add more as i make them.
X_DECLARE_ENUM(LumpType)(Entities, Materials, Planes, Verts, Indexes, Brushes, BrushSides, Surfaces, Nodes, Leafs, Areas, Portals);
X_DECLARE_ENUM(SurfaceType)(Invalid, Plane, Patch);

typedef Flags<MatContentFlags> MatContentFlag;
typedef Flags<MatSurfaceFlags> MatSurfaceFlag;

// I might make this a actualy material definition.
// instead of the name of a material.
// might as well make it internal.
// Hotreload:
//	still work fine, since all materials will end up
//	in the mat manager even if loaded from here.
//	so the entry in the map manger we just be updated.
struct Material
{
	core::StackString<MAP_MAX_MATERIAL_LEN> Name;
	MatSurfaceFlag		surfaceFlag;
	MatContentFlag		contentFlag;
};


// Vertex and index types.
// must be one from vertexForamts.h and one the engine understands.
typedef Vertex_P3F_T4F_N3F_C4B Vertex;
typedef int Index;

// plane type for BSP.
typedef Planef Plane;

// a surface which can be a plane or a patch.
// a plane also uses indexes.
struct Surface
{
	int32_t				materialIdx;
	SurfaceType::Enum	surfaceType; // 8

	int32_t				vertexStartIdx;
	int32_t				numVerts;	// 16

	int32_t				indexStartIdx;
	int32_t				numIndexes;	// 24

	int32_t				lightMapNum;
	Vec2<int32_t>		lightMapPos;
	Vec2<int32_t>		lightMapDimensions;

	// only set for patches.
	Vec2<int32_t>		patchSize;	// 44
}; 


// Used for collision detection, not rendering.
struct BrushSide
{
	int32_t planeIdx;
	int32_t shaderNum;
};

struct Brush
{
	int32_t	firstSide;
	int32_t	numSides;
	int32_t	shaderNum;
};

struct Leaf
{
	int32_t cluster;	//cluster index for visdata
	int32_t area;		// areaportal area

	AABB bounds; // 0x20

	int32_t leafFaceStartIdx;	// first index in leafFaces array
	int32_t numFaces;	
	int32_t leafBrushStartIdx;	// first index into leaf brushes array
	int32_t numBrushes;	// 0x30
};


struct Node
{
	int32_t planeIdx;
	// child nodes,  negative numbers are 'struct Leaf' not nodes.
	int32_t front, back;
	AABB bounds; // 0x24
};

struct Portal
{
	int32_t		areaTo;		// the area this portal leads to.
	XWinding*	pWinding;	// winding points have counter clockwise ordering seen this area
							// should i add seralise support to winding?
							// or i could have a diffrent portal structure for the file.

	Planef		plane;		// view must be on the positive side of the plane to cross
//	Portal*		pNext;		
};

struct Area
{
	int32_t areaNum;
	int32_t numPortals;
	Portal* pPortals;

	AABB bounds; // 0x28
};

// ============ File Structure stuff =========

struct FileLump
{
	uint32_t offset; // no 4gb+ bsp's for you!
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

	core::dateTimeStampSmall modified; // 4

	// crc32 is made from just the lump info.
	// used for reload checks only.
	// -potentialy good for basic integreity checks.
	// -as tricky to change 4 bytes in lump info to forge it.
	// -if we don't allow a offset to be provided without a size that is. (currently allowed)
	// -the crc32 value in the file could not be trusted tho, for obvious reasons.
	uint32_t datacrc32;

	FileLump lumps[LumpType::ENUM_COUNT];

	const bool isValid(void) const {
		return fourCC == BSP_FOURCC;
	}
};


// my nipples don't mess around.
X_ENSURE_SIZE(Material, 76);

X_ENSURE_SIZE(Vertex, 44);
X_ENSURE_SIZE(Index, 4);
X_ENSURE_SIZE(Plane, 16);

X_ENSURE_SIZE(Surface, 0x34);

X_ENSURE_SIZE(BrushSide, 8);
X_ENSURE_SIZE(Brush, 12);

X_ENSURE_SIZE(Leaf, 0x30);
X_ENSURE_SIZE(Node, 0x24);
X_ENSURE_SIZE(Portal, 0x28);
X_ENSURE_SIZE(Area, 0x28);


X_ENSURE_SIZE(FileLump, 8);
X_ENSURE_SIZE(FileHeader, (16 + (8 * LumpType::ENUM_COUNT)));

X_NAMESPACE_END

#endif // !X_BSP_MAP_H_
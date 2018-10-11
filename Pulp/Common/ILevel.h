#pragma once

#ifndef X_BSP_MAP_H_
#define X_BSP_MAP_H_

#include <Math\VertexFormats.h>
#include <String\StackString.h>
#include <Time\CompressedStamps.h>
#include <String\GrowingStringTable.h>

#include <Util\Pointer64.h>
#include <Memory\MemCursor.h>

#include <IRenderMesh.h>
#include <IFileSys.h>
#include <IPhysics.h>
#include <IAssetDb.h>
#include <IEntity.h>

// forward Decs.
template<class Allocator>
class XWindingT;
class WindingGlobalAlloc;

typedef XWindingT<WindingGlobalAlloc> XWinding;

X_NAMESPACE_DECLARE(model,
                    class XModel;)

// ~forward Decs.

X_NAMESPACE_BEGIN(level)

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
//		Each area that has been determined as visable, has it's entiry list processed.
//		When the level was compiled all the ents in the area where added to the area's ent ref list.
//		So I know what is visible in each area.
//
//		We can then perform aditional culling on this list: funcstrum, portal stack.
//
//		Ent's that are in multiple area's are stored / processed diffrently.
//
//		How they are stored:
//
//		We ((areaNum / 32) + 1) lists, each responsible for 64 areas in the map.
//		each item in the list has a 32bit flag for the area's it's in.
//
//		Then we create a flag for the frames visible area's and traverse down the list
//		& the flags and if it's positive the ent is in one of the visible area's
//
//		Example:
//
//		Visible Areas: 1,6,24
//		Flag:    00000001 00000000 00000000 01000010
//
//		0-63 list:
//
//		| 0 | chair_wood | 00000000 00000000 00000000 00000011
//		| 1 | chair_wood | 00000000 00000000 00100100 00000000
//		| 2 | chair_wood | 00000001 00000000 00000000 00000000
//		| 3 | chair_wood | 01100000 00000000 00000000 00000000
//
//		So after going down the flag indexes 0,2 are visible.
//		If a end is visible in area 62 and 79 it will still work fine.
//
//		Lists will only get checked if there is a currently visible area in that list.
//		Aka the lists visibility flag is not zero.
//
//	-----------------------------------------
//
//	Each area in the map is considered to be a model with x surfaces.
//
//	Since each area is classed as a model the XModels in the level format are all that are needed to render
//	the whole map.
//
//	But we store other information that is used for collision and working out where you are in the level.
//	this information is laos used to tell you what area these models are in.
//
//	So once we found out what area we are located in, we add the area for rendering.
//
//
//
//

static const uint32_t LVL_VERSION = 16; //  chnage everytime the format changes. (i'll reset it once i'm doing messing around)
static const uint32_t LVL_FOURCC = X_TAG('x', 'l', 'v', 'l');
static const uint32_t LVL_FOURCC_INVALID = X_TAG('x', 'e', 'r', 'r'); // if a file falid to write the final header, this will be it's FourCC
// feels kinda wrong to call it a '.bsp', since it's otherthings as well.
// '.level' is more pleasing to me and more importantly the BushMaster of Christmas Island(Southeast Asia).
static const char* LVL_FILE_EXTENSION = "level"; // ".bsp";
static const char* LVL_ENT_FILE_EXTENSION = "json";

// a level can not exceed this size.
static const int32_t MAX_WORLD_COORD = (128 * 1024);
static const int32_t MIN_WORLD_COORD = (-128 * 1024);
static const int32_t MAX_WORLD_SIZE = (MAX_WORLD_COORD - MIN_WORLD_COORD);

// some of these limts are done on .map load.
// others checked while compiling bsp.
// slap a hat, swing a rat and hit it with a bat.
static const uint32_t MAP_MAX_PLANES = 65536;
static const uint32_t MAP_MAX_VERTS = 65536;
static const uint32_t MAP_MAX_INDEXES = 65536;
static const uint32_t MAP_MAX_BRUSHES = 32768;
static const uint32_t MAP_MAX_BRUSHSIDES = 65536;   // total sides in map / bsp
static const uint32_t MAP_MAX_SIDES_PER_BRUSH = 64; // max sides a single brush can have.
static const uint32_t MAP_MAX_NODES = 65536;
static const uint32_t MAP_MAX_LEAFS = 65536;
static const uint32_t MAP_MAX_AREAS = 0x100;
static const uint32_t MAP_MAX_MULTI_REF_LISTS = (MAP_MAX_AREAS / 32);
static const uint32_t MAP_MAX_PORTALS = 0x100;
static const uint32_t MAP_MAX_SURFACES = 65536 * 2;
static const uint32_t MAP_MAX_MODELS = 0x400;             // a model is a 'area'.
static const uint32_t MAP_MAX_MODEL_SURFACES = 65536 * 2; // the maximum surfaces a map model can have.
static const uint32_t MAP_MAX_MODEL_VERTS = 65536;        // the maximum verts a map model can have.
static const uint32_t MAP_MAX_MODEL_INDEXES = 65536;      // the maximum indexes a map model can have.
static const uint32_t MAP_MAX_MATERIALS = 0x800;          // this is for level geo only

// area col stuff
static const uint32_t MAP_MAX_AREA_COL_GROUPS = 0xff;                                    // the max amount of col groups for a given area
static const uint32_t MAP_MAX_AREA_COL_DATA = 0xff;                                      // the max amount of col data for a arena, aka the sum of all data types for that area (triMesh, hieghtField)
static const uint32_t MAP_MAX_AREA_COL_DATA_SIZE = std::numeric_limits<uint16_t>::max(); // the max size of a single col data

// Should be checked in compiler.
static const uint32_t MAP_MAX_ENTITES = 0x400;
static const uint32_t MAP_MAX_LIGHTS_WORLD = 4096;

// might be removed in-favor of embeded binary materials
static const uint32_t MAP_MAX_MATERIAL_LEN = assetDb::ASSET_NAME_MAX_LENGTH;

// Key / Value limits
static const uint32_t MAX_KEY_LENGTH = 64;    // KVP: name
static const uint32_t MAX_VALUE_LENGTH = 256; // KVP: value

// show me the light, o holy one.
static const uint32_t LIGHT_MAP_WIDTH = 128;
static const uint32_t LIGHT_MAP_HEIGHT = 128;

X_DECLARE_FLAGS(MatContentFlags)
(
    SOLID,
    WATER,
    PLAYER_CLIP,
    MONSTER_CLIP,
    TRIGGER,
    NO_FALL_DMG,
    DETAIL,
    STRUCTURAL,
    ORIGIN);

X_DECLARE_FLAGS(MatSurfaceFlags)
(
    NO_DRAW,
    LADDER);

// this is the flags for the file header, which tells you what option stuff is inside the file.
X_DECLARE_FLAGS(LevelFileFlag)
(
    INTER_AREA_INFO,
    AREA_STATIC_MODEL_REF_LISTS,
    BSP_TREE,
    OCT_TREE,
    DEBUG_PORTAL_DATA,
    COLLISION,
    COMPRESSED);

typedef Flags<LevelFileFlag> LevelFileFlags;

X_DECLARE_ENUM(SurfaceType)
(
    Invalid,
    Plane,
    Patch);
X_DECLARE_ENUM(Side)
(
    FRONT,
    BACK);

typedef Flags<MatContentFlags> MatContentFlag;
typedef Flags<MatSurfaceFlags> MatSurfaceFlag;

typedef core::GrowingStringTable<256, 16, 4, uint32_t> StringTable;
typedef core::GrowingStringTableUnique<256, 16, 4, uint32_t> StringTableUnique;

// I might make this a actualy material definition.
// instead of the name of a material.
// might as well make it internal.
// Hotreload:
//	still work fine, since all materials will end up
//	in the mat manager even if loaded from here.
//	so the entry in the map manger we just be updated.
struct Material
{
    // move this to a string table.
    core::StackString<MAP_MAX_MATERIAL_LEN> Name;
    MatSurfaceFlag surfaceFlag;
    MatContentFlag contentFlag;
};

// Vertex and index types.
// must be one from vertexForamts.h and one the engine understands.
typedef Vertex_P3F_T4F VertexBase;
typedef Vertex_P3F_T4F_C4B_N3F Vertex;
typedef int Index;

// plane type for BSP.
typedef Planef Plane;

#if 0
struct Entity
{
	Vec3f pos;

};

struct StaticModel : public Entity
{
	uint16_t modelNameIdx; // string table.

	core::Pointer64<model::IRenderMesh > pRenderMesh;
};
#else

// not saved to file sruntime only.
struct StaticModel
{
    Transformf transform;

    // bounding box that takes into account the angle.
    AABB boundingBox;
    Sphere boundingSphere;

    uint32_t modelNameIdx;
    model::XModel* pModel;
};

#endif

// ============ File Structure stuff =========

// WIP: format is currently been changed.
//
// One question:
//
//	How do i want to store all the entities.
//	the problem is that we support many diffrent kvp's for some ent's
//	so i can't really have a structure that represents that in binary.
//	so what i was thinking was have static models represented by binary layout
//	since there will be so many of them compared to other ents.

X_DECLARE_ENUM(FileNodes)
(
    STRING_TABLE,
    AREA_MODELS, // the model data for each area.
    AREA_PORTALS,
    AREA_STATIC_MODEL_REFS, // area model refs.
    AREA_COLLISION,         // the collision data for each area.
    STATIC_MODELS,          // all the static models in the map.
    BSP_TREE,
    ENTITIES,
    LIGHTS_STATIC,
    LIGHTS_DYNAMIC);

struct EnityInfoHdr
{
    uint32_t dataSize[game::ClassType::ENUM_COUNT]; // the size of the data for the given class type.
};

struct FileAreaRefHdr
{
    uint32_t startIndex;
    uint32_t num;
};

X_PACK_PUSH(4)
struct AreaEntRef
{
    uint32_t modelId;
};

struct MultiAreaEntRef : public AreaEntRef
{
    uint32_t flags;
};
X_PACK_POP

struct FileStaticModel
{
    Vec3f pos;
    Quatf angle;

    // storing a precomputed AABB here would be nice.
    // that takes into account rotation of the model.
    // this could either be done fast by using the AABB or slow and more accurate
    // using rotated verts.
    AABB boundingBox;

    uint32_t modelNameIdx;
};

X_DECLARE_ENUM8(CollisionDataType)
(
    TriMesh,
    ConvexMesh,
    HeightField,
    Aabb);

X_PACK_PUSH(1)

struct AreaCollisionHdr
{
    uint8_t numGroups;
    uint8_t _pad[3];
    // all shapes are relative to this.
    Vec3f trans;
};

struct AreaCollisionGroupHdr
{
    // the flags for this group
    physics::GroupFlags groupFlags;
    // the number of each type.
    uint16_t numTypes[CollisionDataType::ENUM_COUNT];
};

// one for each type.
struct AreaCollisionDataHdr
{
    uint16_t dataSize;
};

// some checks for col types.
static_assert(std::numeric_limits<decltype(AreaCollisionHdr::numGroups)>::max() >= MAP_MAX_AREA_COL_GROUPS, "Limit exceeds type max");
static_assert(std::numeric_limits<std::remove_reference<decltype(AreaCollisionGroupHdr::numTypes[0])>::type>::max() >= MAP_MAX_AREA_COL_DATA, "Limit exceeds type max");
static_assert(std::numeric_limits<decltype(AreaCollisionDataHdr::dataSize)>::max() >= MAP_MAX_AREA_COL_DATA_SIZE, "Limit exceeds type max");

X_ENSURE_SIZE(AreaCollisionHdr, 16);
X_ENSURE_SIZE(AreaCollisionGroupHdr, 12);
X_ENSURE_SIZE(AreaCollisionDataHdr, 2);

X_DECLARE_ENUM8(LightType)
(
    Point,
    Spot);

struct Light
{
    Vec3f pos;
    Quatf angle;

    Colorf col;
    LightType::Enum type;
};

X_PACK_POP

struct FileNode
{
    FileNode()
    {
        core::zero_this(this);
    }

    X_INLINE bool isSet(void) const
    {
        return size > 0;
    }

    uint32_t size;
    uint32_t offset;
};

struct FileHeader
{
    uint32_t fourCC;
    uint8_t version;
    uint8_t blank[3];

    LevelFileFlags flags;

    core::DateTimeStampSmall modified; // 4

    // size of all nodes
    uint32_t totalDataSize;

    // crc32 is just the header data
    uint32_t datacrc32;

    // stirng table data.
    uint32_t numStrings;

    // the number of area;s in the level file.
    // also signifys how many multi area ref ent lists we have. (num / 32) + 1
    int32_t numAreas;
    int32_t numinterAreaPortals;
    int32_t numNodes;

    int32_t pad_1;
    int32_t pad_2;

    // model ref sizes.
    int32_t numModelRefs;
    int32_t numMultiAreaModelRefs;
    // size of the static model info.
    int32_t numStaticModels;

    FileNode nodes[FileNodes::ENUM_COUNT];

    uint32_t pad; // make it multiple of 16.

    const bool isValid(void) const
    {
        return fourCC == LVL_FOURCC;
    }

    core::MemCursor memCursorForNode(uint8_t* pData, FileNodes::Enum node) const
    {
        return core::MemCursor(pData + nodes[node].offset, nodes[node].size);
    }

    core::XFileFixedBuf FileBufForNode(uint8_t* pData, FileNodes::Enum node) const
    {
        uint8_t* pBegin = pData + nodes[node].offset;
        return core::XFileFixedBuf(pBegin, pBegin + nodes[node].size);
    }
};

// my nipples don't mess around.
//X_ENSURE_SIZE(Material, 76);

//X_ENSURE_SIZE(Vertex, 44);
//X_ENSURE_SIZE(Index, 4);
//X_ENSURE_SIZE(Plane, 16);

// X_ENSURE_SIZE(Portal, 0x28);
//X_ENSURE_SIZE(Area, 0x28 + 0xC + 12);

X_ENSURE_SIZE(EnityInfoHdr, 4 * game::ClassType::ENUM_COUNT);

X_ENSURE_SIZE(FileAreaRefHdr, 8);
X_ENSURE_SIZE(AreaEntRef, 4);
X_ENSURE_SIZE(MultiAreaEntRef, 8);

// check file structure sizes also.
X_ENSURE_SIZE(FileStaticModel, 56);
X_ENSURE_SIZE(FileNode, 8);
X_ENSURE_SIZE(FileHeader, 64 + (sizeof(FileNode) * FileNodes::ENUM_COUNT));

X_NAMESPACE_END

#endif // !X_BSP_MAP_H_

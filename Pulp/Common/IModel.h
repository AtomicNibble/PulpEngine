#pragma once


#ifndef _X_ASSET_MODEL_H_
#define _X_ASSET_MODEL_H_

#include <Util\FlagsMacros.h>
#include <Util\Pointer64.h>

#include <String\StackString.h>
#include <String\CmdArgs.h>

#include <Math\XVector.h>
#include <Math\XAabb.h>
#include <Math\XQuatCompressed.h>
#include <Math\XVecCompressed.h>
#include <Math\XHalf.h>
#include <Math\VertexFormats.h>

#include <Time\CompressedStamps.h>

#include <IMaterial.h>
#include <IConverterModule.h>

X_NAMESPACE_BEGIN(model)

// The Model Foramts
//
//  File Ext: .model
//	Version: 10.0 (i keep adding features / improving :( )
//  Info:
//  
//  This format contains the model info.
//	We only support convex pologons.
//
//	Limits:
//		max bones:				255 (includes blanks)
//		Max verts per mesh:		(1 << 16)-1
//		Max mesh per LOD:		64
//		Max LOD:				4
// 
//	Vert Limits:
//		Max binds:				4
//		
//		
// Structure:
//
//		ModelHeader
//		LodInfo[MAX_LOD]
//
//		MaterialNames[NumMesh] // nullterm (block read)
//
//		BoneNames[NumBones] // no names stored for blanks.
//		BoneNameTableIdx[NumBones + NumBlank] (16bit)
//		BoneTree[NumBones]
//		BoneAngles[NumBones]
//		BonePos[NumBones]
//
//		MeshHeaders[numMesh] // all lods
//
//		FOR EACH LOD:
//			Verts[AllMesh] <- version 8 adds streams, but vert data is not considered a optional stream.
//			Streams[AllMesh][Numstreams]
//			Faces[AllMesh]
//			BindData[AllMesh]
//		END
//
//
//	Version 6:
//		renamed some shit, and refactored some struts.
//
//	Version 7:
//		changed vertex format.
//	
//	Version 8: 
//		Adds supports for streams.
//		models with the fullVertex flag don't contain streams.
//
//		The base vertex format has changed, and is just position and textcoords.
//		Anything else is put into a stream.
//	
//		Since I will not need all the info all the time.
//
//		Streams in the file appear in the same order as the flags.
//
//  Version 9:
//			??
//
//	Version 10:
//			Includes combined AABB in header.
//
//	Version 11:
//		Add vertex format to the model file Header. also force 4 byte padding.
//
//	Version 12:
//		Bump model version to signify the flip uv and reverse face winding.
//
//	Version 13:
//		Align all the vertex streams to 16byte offsets, so that when we map the file to memory.
//		As long as the base memory location of the buffer is 16byte alinged all the stream starts will be also.
//		This then allows SIMD memcpy on the streams in the render system, when uploading.
//
//	Version 14:
//		Add support for physics data.
//		Physics data is stored if PHYS_DATA flag is set, and it's stored directly after the bone data.
//		
//		The whole block can be skipped by seeking hdr.physDataSize
//		
//		Layout:
//		
//			CollisionInfoHdr colHdr
//			uint8_t 8bitIndexMap[numShapes]
//	
//			Sphere[colHdr.numShpere]
//			AABB[colHdr.numBox]
//			{
//				CollisionConvexHdr
//				convexDataBlob
//			} [colHdr.numConvex]
//				
//	Version 15:
//		Add support for hitbox data.
//		HitBox data is stored if HITBOX flag is set, and it's stored directly after bone / phys data.
//	
//		The whole block can be skipped by seeking (hdr.hitboxDataBlocks * 64)
//
//		Layout:
//			HitBoxHdr
//			uint8_t boneIdxMap[numShapes]
//			Sphere[colHdr.numShpere]
//			OBB[colHdr.numOBB]
//
//

#define X_MODEL_BONES_LOWER_CASE_NAMES 1
#define X_MODEL_MTL_LOWER_CASE_NAMES 1

static const uint32_t	 MODEL_VERSION = 15;
static const uint32_t	 MODEL_MAX_BONES = 255;
static const uint32_t	 MODEL_MAX_BONE_NAME_LENGTH = 64;
static const uint32_t	 MODEL_MAX_MESH = 64;
static const uint32_t	 MODEL_MAX_VERTS = (1 << 16) - 1;
static const uint32_t	 MODEL_MAX_INDEXS = MODEL_MAX_VERTS;
static const uint32_t	 MODEL_MAX_FACES = MODEL_MAX_INDEXS / 3;
static const uint32_t	 MODEL_MAX_COL_SHAPES = 255; // max shapes per model, there is a per mesh limit also MODEL_MESH_COL_MAX_MESH
static const uint32_t	 MODEL_MAX_COL_DATA_SIZE = std::numeric_limits<uint16_t>::max(); // max size of all phys data.
static const uint32_t	 MODEL_MAX_HITBOX_DATA_SIZE = MODEL_MAX_BONES * 64;

// humm might make this 8 (would be for faces, probs make it a option)
// I've made it 8 for the format, but i'm gonna make it so you need to turn on 8vert mode for compiler.
static const uint32_t	 MODEL_MAX_VERT_BINDS = 8;
static const uint32_t	 MODEL_MAX_VERT_BINDS_NONE_EXT = 4;

static const uint32_t	 MODEL_MAX_LODS = 4;
// legnth checks are done without extension, to make it include extension, simple reduce it by the length of ext.
static const uint32_t	 MODEL_MAX_NAME_LENGTH = 60; 
static const char*		 MODEL_FILE_EXTENSION = "model";
static const wchar_t*	 MODEL_FILE_EXTENSION_W = L"model";
static const char*		 MODEL_DEFAULT_NAME = "default/default";

// max col meshes per mesh


// Intermidiate format stuff.
// this is used for saving out raw un processed data. so no dropping of bones, or weights etc..
// Also text based allowing for other tools to create it with ease.
// I plan to also have the maya plugin background save one so that we have one of these for every model.
// Allowing for all models to be reconverted without quat compression on the angles etc.
static const uint32_t	 MODEL_RAW_VERSION = 1;
static const char*		 MODEL_RAW_FILE_EXTENSION = "model_raw";


// col mesh info
static const char		 MODEL_MESH_COL_BOX_PREFIX[] = "PBX_";
static const char		 MODEL_MESH_COL_SPHERE_PREFIX[] = "PSP_";
static const char		 MODEL_MESH_COL_CONVEX_PREFIX[] = "PCX_";
static const uint32_t	 MODEL_MESH_COL_MAX_VERTS = 255; // source and baked limit, this is only for convex, for aabb / sphere i don't care.
static const uint32_t	 MODEL_MESH_COL_MAX_FACE = 255;
static const uint32_t	 MODEL_MESH_COL_MAX_COOKED_SIZE = std::numeric_limits<uint16_t>::max();
static const uint32_t	 MODEL_MESH_COL_MAX_MESH = 8; // max col mesh per a mesh. (yes we allow multiple col meshes for each mesh)



static const uint32_t	 MODEL_MAX_LOADED = 1 << 10; // max models that can be loaded.



struct IModelLib : public IConverter
{

};


struct IModel
{
	virtual ~IModel() {}


};


// Flags:
// LOOSE: the model file is not packed, meaning it has a string table that needs processing.
// FULL_VERT: the vertex data is not compressed.
// STREAMS: more than just vertex stream. (a stream info block is included)
// ANIMATED: model has bind data.
// PHYS_DATA: this model contains collision shapes.
// PHYS_BAKED: the convex meshes are baked.
X_DECLARE_FLAGS8(ModelFlag)(LOOSE, FULL_VERT, STREAMS, ANIMATED, PHYS_DATA, PHYS_BAKED, HITBOX);
X_DECLARE_FLAGS(MeshFlag)(
	SYS_MEMORY, 
	VIDEO_MEMORY, 
	// nothing sets this or checks this, i need to decide how i want to implement the 32bit index support.
	// per lod or per mesh?
	// can i use 16bit indexes for a lod if all sub mesh have less than 65k verts?
	// depends how i handle drawing etc.
	BIG_INDEX,
	ANIMATED // the mesh has binds.
); 

X_DECLARE_ENUM(ColGenType)(
	SPHERE,
	BOX,
	CAPSULE,
	PER_MESH_SPHERE,
	PER_MESH_BOX,
	KDOP_6,
	KDOP_14,
	KDOP_18,
	KDOP_26
);

X_DECLARE_ENUM(ColMeshType)(
	SPHERE, 
	BOX, 
	// CAPSULE,
	CONVEX
);

X_DECLARE_ENUM(HitBoxType)(
	SPHERE,
	OBB
);

// VertexStream from vertexformats.h is has a copy also, but it is enum not flags.
// we have flags here for checking what streams are provided.
// vert is always provided.
// streams appear in same order as the flags
X_DECLARE_FLAGS8(StreamType)(COLOR, NORMALS, TANGENT_BI);
X_DECLARE_ENUM8(StreamFmt)(VERT,VERT_FULL,VEC2,VEC216,VEC3,VEC3COMP);

typedef Flags8<ModelFlag> ModelFlags;
typedef Flags<MeshFlag> MeshFlags;
typedef Flags8<StreamType> StreamTypeFlags;

X_DECLARE_FLAG_OPERATORS(ModelFlags);
X_DECLARE_FLAG_OPERATORS(MeshFlags);
X_DECLARE_FLAG_OPERATORS(StreamTypeFlags);


X_PACK_PUSH(1)

struct bindWeight
{
	X_INLINE bindWeight() : compressed_(0) {}

	X_INLINE bindWeight(float val) {
		compressed_ = static_cast<uint16_t>(val * 65535.f);
	}

	X_INLINE float getWeight(void) const {
		return static_cast<float>(compressed_) / 65535.0f;
	}

	X_INLINE operator uint16_t(void) const {
		return compressed_;
	}

private:
	uint16_t compressed_; // scaled
};

struct bindBone
{
	X_INLINE bindBone() : compressed_(0) {}
	X_INLINE bindBone(uint16_t id) : compressed_(id * 128) {
		X_ASSERT(static_cast<uint32_t>(id) * 128 < std::numeric_limits<uint16_t>::max(), 
			"Bone id scaled exceeds type limits")(id);
	}

	X_INLINE uint8_t getIndex(void) const {
		return safe_static_cast<uint8_t>(compressed_ / 128);
	}

	// used for sorting
	X_INLINE const int operator-(const bindBone& tag) const {
		return compressed_ - tag.compressed_;
	}
	X_INLINE bool operator==(const bindBone& tag) const {
		return tag.compressed_ == compressed_;
	}
private:
	uint16_t compressed_; // scaled
};


// used for when the mesh has no complex binds.
// a mesh can have 1 or more of these allowing for multiple simple binds to a mesh
struct simpleBind
{
	uint16_t jointIdx; // 16bit for padding.
	uint16_t numVerts;
	uint16_t startVert;
	uint16_t _pad;
	// uint16_t numFaces;
	// uint16_t faceOffset;
};

// each vertex supports between 1-8 binds.
struct singleBind
{
	bindBone	bone1;
};

struct doubleBind
{
	bindBone	bone1;
	bindBone	bone2;
	bindWeight	weight2;
};

struct trippleBind : public doubleBind
{
	bindBone	bone3;
	bindWeight	weight3;
};

struct quadBind : public trippleBind
{
	bindBone	bone4;
	bindWeight	weight4;
};

struct fiveBind : public quadBind
{
	bindBone	bone5;
	bindWeight	weight5;
};

struct sixBind : public fiveBind
{
	bindBone	bone6;
	bindWeight	weight6;
};

struct sevenBind : public sixBind
{
	bindBone	bone7;
	bindWeight	weight7;
};

struct eightBind : public sevenBind
{
	bindBone	bone8;
	bindWeight	weight8;
};

X_PACK_POP


// base vertex is just pos + texcords.
// anything else is provided by streams.
typedef Vertex_P3F_T2S Vertex;

typedef Color8u VertexColor;
typedef Vec3f VertexNormal;
typedef compressedVec3 VertexNormalCompressed;
typedef Vertex_Tangents_BiNorm	VertexTangentBi;
typedef Vertex_Tangents_BiNorm_Compressed VertexTangentBiCompressed;

// a lossless layout
struct VertexFull
{
	Vec3f			pos;
	Vec2f			uv;
	VertexColor		col;
	VertexNormal	normal;
	Vec3f			tangent;
	Vec3f			binormal;
	uint8_t			_pad[4];
}; // 64 bytes

X_ENSURE_SIZE(VertexColor, 4);
X_ENSURE_SIZE(VertexNormal, 12);
X_ENSURE_SIZE(VertexNormalCompressed, 4);
X_ENSURE_SIZE(VertexTangentBi, 24);
X_ENSURE_SIZE(VertexTangentBiCompressed, 8);

typedef uint16_t Index;
typedef Vec3<Index> Face;


struct CompbindInfo
{
	typedef std::array<uint16_t, MODEL_MAX_VERT_BINDS> BindCountsArr;

	X_INLINE CompbindInfo() {
		core::zero_object(compBinds_);
	}

	X_INLINE void set(const BindCountsArr& counts) {
		compBinds_ = counts;
	}

	X_INLINE uint16_t& operator[](size_t idx) {
		return compBinds_[idx];
	}

	X_INLINE const uint16_t operator[](size_t idx) const {
		return compBinds_[idx];
	}

	X_INLINE const size_t dataSize(size_t idx) const {
		// size is (idx * (bindBone + bindWeight)) + bindBone
		return compBinds_[idx] * ((idx * (sizeof(bindBone) + sizeof(bindWeight))) + sizeof(bindBone));
	}

	X_INLINE const size_t dataSizeTotal(void) const {
		size_t i, size = 0; 
		for (i = 0; i < MODEL_MAX_VERT_BINDS; i++) {
			size += dataSize(i);
		}
		return size;
	}

	X_INLINE const BindCountsArr& getBindCounts(void) const {
		return compBinds_;
	}

	X_INLINE const bool hasData(void) const {
		return setCheck_[0] != 0 || setCheck_[1] != 0;
	}

private:
	union {
		BindCountsArr compBinds_;
		uint64_t setCheck_[2];
	};
};


// one of these is stored if there is physics data.
// all the colMesh shapes are store in enum order and the counts below show how many.
struct CollisionInfoHdr
{
	std::array<uint8_t, ColMeshType::ENUM_COUNT> shapeCounts; // this is the number of each type we have, they appear in order.
	// we have total shapes * 8bit, this is used to covert the colmeshIndex into the index of the sorted shapes.
	// this is a map from ColMesh to visible mesh index for lod0. -1 is whole lod.
	int8_t idxMap[1]; 

	size_t totalShapes(void) const {
		return core::accumulate(shapeCounts.begin(), shapeCounts.end(), 0_sz);
	}
};

// each convex mesh data block is prefixed with this header.
struct CollisionConvexHdr
{
	// if convex mesh is baked or not is a per model flag.
	// having only a subset baked is not supported.
	union
	{
		struct 
		{
			// when !isBaked these are raw vert and face counts.
			// we have a limit of 255 so 8 bits is fine here.
			uint8_t numVerts;
			uint8_t numFace;
		}raw;

		// when isBaked it's the blob size.
		uint16_t dataSize;
	};
};


// Hitboxes.
//	 There is a HitBoxHdr, followed by a 8bit bone index map.	
//	 followed by the shapes in enum order.
//

struct HitBoxHdr
{
	uint8_t shapeCounts[HitBoxType::ENUM_COUNT];
};


// SubMeshHeader is part of a single mesh.
// each SubMeshHeader typically has a diffrent material.
// the submesh provides vertex / index Offsets, for the verts.
// in the meshes IB / VB
struct SubMeshHeader
{
	SubMeshHeader() {
		numVerts = 0;
		numIndexes = 0;
		numBinds = 0;
		numColMesh = 0;

		startVertex = (uint32_t)-1;
		startIndex = (uint32_t)-1;
	}

	// sizes
	uint16_t numVerts;
	uint16_t numIndexes;
	uint16_t numBinds;	// used for solid binds.
	Flags8<StreamType> streamsFlag; // this is just a dublicate of the meshheaders value, never diffrent.
	uint8_t numColMesh; // 8

	// offset into the mesh's streames.
	uint32_t startVertex;
	uint32_t startIndex; // 8

	// needs to be void, since we have diffrent vertex formats.
	core::Pointer64<void>		streams[VertexStream::ENUM_COUNT - VERT_RUNTIME_STREAM_COUNT]; // 8
	core::Pointer64<Index>		indexes; // 8


	// 16
	union {
		core::Pointer64<engine::Material> pMat;
		core::Pointer64<const char> materialName;
	};

	/* cold fields down here, shizz that's not 
		accessed much. */

	// bind sizes.
	CompbindInfo CompBinds; // 8

	// Bounds for the sub mesh
	AABB boundingBox;
	Sphere boundingSphere;

//	X_NO_COPY(SubMeshHeader);
//	X_NO_ASSIGN(SubMeshHeader);
}; // 128



// a mesh is a single Vb / IB
// It can have many sub-meshes.
// for 3D Models it's limits are above.
// level models have diffrent limits. <IBsp.h>
// respect both when changing type sizes.
struct MeshHeader
{
	MeshHeader() :
		numSubMeshes(0),
		_blank(0),
		numVerts(0),
		numIndexes(0),
		__blank(0)
	{
		core::zero_object(__pad);
	}

	// 4
	uint16_t	numSubMeshes; // levels support 65k sub meshes
	StreamTypeFlags streamsFlag; // flags for what streams are avalible
	uint8_t	_blank;

	// 8 total of all sub-meshes.
	// sub meshes can have upto 65k verts, this is the cumlative count
	// we allow 65k sub meshes, so 32bits is enoungth to fit 65k*65k
	uint32_t    numVerts;
	uint32_t	numIndexes;

	// 4 + 4
	MeshFlags flags;
	uint32_t __blank;

	// 8
	core::Pointer64<SubMeshHeader> subMeshHeads;

	// Version 5, pointer to meshes verts / faces.
	// all sub meshes / index verts are stored contiguous
	// core::Pointer64<void>		verts; // version 8.0 adds streams, verts stream is index 0.
	core::Pointer64<Index>		indexes; // 8

	// 16
	core::Pointer64<void>       streams[VertexStream::ENUM_COUNT - VERT_RUNTIME_STREAM_COUNT];

	uint32_t __pad[2];

	// 24
	AABB boundingBox;	
	Sphere boundingSphere;


//	X_NO_COPY(MeshHeader);
//	X_NO_ASSIGN(MeshHeader);
};


// a lod is a mesh with a distance
struct LODHeader : public MeshHeader
{
	LODHeader() :
	lodDistance(0.f)
	{}

	float lodDistance;
};


X_PACK_PUSH(4)

struct ModelHeader // File header.
{
	uint8_t version;
	ModelFlags flags;
	uint8_t numBones;
	uint8_t numBlankBones;
	uint8_t numMesh;
	uint8_t numLod;
	uint16_t boneDataSize;	// size of the bone data (nameIdx, hierarchy,angles,pos)

	core::dateTimeStampSmall modified; // 4

	uint32_t dataSize;				// filesize - sizeof(ModelHeader).
	uint32_t meshDataSize;			// mesh heads + meshdata + bone data(not names)
	uint16_t materialNameDataSize;	// size of material name block
	uint16_t tagNameDataSize;		// size of tag name block.


	uint16_t physDataSize;
	uint8_t hitboxDataBlocks; // multiply by 64 to get byte size.

	// the format of the merged streams.
	render::shader::VertexFormat::Enum vertexFmt;

	// this is all the lod aabb's merged
	// it's here so we can get bounds for model just from loading header with one read.
	AABB boundingBox;

	LODHeader lodInfo[MODEL_MAX_LODS];

	// definitions in 3DEngine::ModelLoader.cpp
	bool isValid(void) const;
};

X_PACK_POP

// check sizes., what ever is using these structs
// needs to be compiling these as same size so they can be loaded correct.
// the runtime pointers are wrapped to make sure always 64bits.
// so models work in both 32/64bit engine.
// while also allows the tool making the file to be 32bit.
X_ENSURE_SIZE(compressedVec3, 4);
X_ENSURE_SIZE(simpleBind, 8);

X_ENSURE_SIZE(bindBone, 2);
X_ENSURE_SIZE(bindWeight, 2);

X_ENSURE_SIZE(doubleBind, 6);
X_ENSURE_SIZE(trippleBind, 10);
X_ENSURE_SIZE(quadBind, 14);
X_ENSURE_SIZE(fiveBind, 18);
X_ENSURE_SIZE(sixBind, 22);
X_ENSURE_SIZE(sevenBind, 26);
X_ENSURE_SIZE(eightBind, 30);


X_ENSURE_SIZE(Vertex, 16);
X_ENSURE_SIZE(Face, 6);

X_ENSURE_SIZE(SubMeshHeader, 128);
X_ENSURE_SIZE(MeshHeader, 128);
X_ENSURE_SIZE(LODHeader, sizeof(MeshHeader)+8);
X_ENSURE_SIZE(ModelHeader, (sizeof(LODHeader)*MODEL_MAX_LODS) + 52);


X_ENSURE_SIZE(CollisionInfoHdr, 4);
X_ENSURE_SIZE(CollisionConvexHdr, 2);


X_NAMESPACE_END

#endif // !_X_ASSET_MODEL_H_
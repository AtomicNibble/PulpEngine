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
//

#define X_MODEL_BONES_LOWER_CASE_NAMES 1
#define X_MODEL_MTL_LOWER_CASE_NAMES 1

static const uint32_t	 MODEL_VERSION = 11;
static const uint32_t	 MODEL_MAX_BONES = 255;
static const uint32_t	 MODEL_MAX_BONE_NAME_LENGTH = 64;
static const uint32_t	 MODEL_MAX_MESH = 64;
static const uint32_t	 MODEL_MAX_VERTS = (1 << 16) - 1;
static const uint32_t	 MODEL_MAX_INDEXS = MODEL_MAX_VERTS;
static const uint32_t	 MODEL_MAX_FACES = MODEL_MAX_INDEXS / 3;

// humm might make this 8 (would be for faces, probs make it a option)
// I've made it 8 for the format, but i'm gonna make it so you need to turn on 8vert mode for compiler.
static const uint32_t	 MODEL_MAX_VERT_BINDS = 8;
static const uint32_t	 MODEL_MAX_VERT_BINDS_NONE_EXT = 4;

static const uint32_t	 MODEL_MAX_LODS = 4;
// legnth checks are done without extension, to make it include extension, simple reduce it by the length of ext.
static const uint32_t	 MODEL_MAX_NAME_LENGTH = 60; 
static const char*		 MODEL_FILE_EXTENSION = "model";
static const wchar_t*	 MODEL_FILE_EXTENSION_W = L"model";

// max col meshes per mesh
static const uint32_t	MODEL_MAX_COL_MESH = 8;


// Intermidiate format stuff.
// this is used for saving out raw un processed data. so no dropping of bones, or weights etc..
// Also text based allowing for other tools to create it with ease.
// I plan to also have the maya plugin background save one so that we have one of these for every model.
// Allowing for all models to be reconverted without quat compression on the angles etc.
static const uint32_t	 MODEL_RAW_VERSION = 1;
static const char*		 MODEL_RAW_FILE_EXTENSION = "model_raw";


static const char		 MODEL_MESH_COL_BOX_PREFIX[] = "PBX_";
static const char		 MODEL_MESH_COL_SPHERE_PREFIX[] = "PSP_";
static const char		 MODEL_MESH_COL_CONVEX_PREFIX[] = "PCX_";


struct IModelLib : public IConverter
{

};



// Flags:
// LOOSE: the model file is not packed, meaning it has a string table that needs processing.
// FULL_VERT: the vertex data is not compressed.
// STREAMS: more than just vertex stream. (a stream info block is included)
// ANIMATED: model has bind data.
X_DECLARE_FLAGS8(ModelFlags)(LOOSE, FULL_VERT, STREAMS, ANIMATED);
X_DECLARE_FLAGS(MeshState)(SYS_MEMORY, VIDEO_MEMORY);

// VertexStream from vertexformats.h is has a copy also, but it is enum not flags.
// we have flags here for checking what streams are provided.
// vert is always provided.
// streams appear in same order as the flags
X_DECLARE_FLAGS8(StreamType)(COLOR, NORMALS, TANGENT_BI);
X_DECLARE_ENUM8(StreamFmt)(VERT,VERT_FULL,VEC2,VEC216,VEC3,VEC3COMP);


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

	X_INLINE uint16_t getIndex(void) const {
		return compressed_ / 128;
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
	uint16_t numFaces;
	uint16_t faceOffset;
};

// each vertex supports between 1-8 binds.
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

	X_INLINE uint16_t& operator[](int idx) {
		X_ASSERT(idx >= 0 && idx < MODEL_MAX_VERT_BINDS, "index out of range")(idx, MODEL_MAX_VERT_BINDS);
		return compBinds_[idx];
	}

	X_INLINE const uint16_t operator[](int idx) const {
		X_ASSERT(idx >= 0 && idx < MODEL_MAX_VERT_BINDS, "index out of range")(idx, MODEL_MAX_VERT_BINDS);
		return compBinds_[idx];
	}

	X_INLINE const size_t dataSize(int idx) const {
		X_ASSERT(idx >= 0 && idx < MODEL_MAX_VERT_BINDS, "index out of range")(idx, MODEL_MAX_VERT_BINDS);
		// size is (idx * (bindBone + bindWeight)) + bindBone
		return compBinds_[idx] * ((idx * (sizeof(bindBone) + sizeof(bindWeight))) + sizeof(bindBone));
	}

	X_INLINE const size_t dataSizeTotal(void) const {
		size_t size = 0; int i;
		for (i = 0; i < MODEL_MAX_VERT_BINDS; i++) {
			size += dataSize(i);
		}
		return size;
	}

	X_INLINE const BindCountsArr& getBindCounts(void) const {
		return compBinds_;
	}

private:
	BindCountsArr compBinds_;
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
		_unused = 0;

		startVertex = (uint32_t)-1;
		startIndex = (uint32_t)-1;
	}

	// sizes
	uint16_t numVerts;
	uint16_t numIndexes;
	uint16_t numBinds;	// used for solid binds.
	Flags8<StreamType> streamsFlag; // this is just a dublicate of the meshheaders value, never diffrent.
	uint8_t _unused; // 8

	// offset into the mesh's streames.
	uint32_t startVertex;
	uint32_t startIndex; // 8

	// needs to be void, since we have diffrent vertex formats.
	core::Pointer64<void>		streams[VertexStream::ENUM_COUNT]; // 8
	core::Pointer64<Index>		indexes; // 8


	// 16
	core::Pointer64<engine::IMaterial> pMat;
	core::Pointer64<const char> materialName;

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
		numVerts(0),
		numIndexes(0),
		_blank(0),
		__blank(0)
	{
		core::zero_object(__pad);
	}

	// 4
	uint16_t	numSubMeshes; // levels support 65k sub meshes
	Flags8<StreamType> streamsFlag; // flags for what steams are avalible
	uint8_t	_blank;

	// 8 total of all sub-meshes.
	// sub meshes can have upto 65k verts, this is the cumlative count
	// we allow 65k sub meshes, so 32bits is enoungth to fit 65k*65k
	uint32_t    numVerts;
	uint32_t	numIndexes;

	// 4 + 4
	Flags<MeshState> state;
	uint32_t __blank;

	// 8
	core::Pointer64<SubMeshHeader> subMeshHeads;

	// Version 5, pointer to meshes verts / faces.
	// all sub meshes / index verts are stored contiguous
	// core::Pointer64<void>		verts; // version 8.0 adds streams, verts stream is index 0.
	core::Pointer64<Index>		indexes; // 8

	// 16
	core::Pointer64<void>       streams[VertexStream::ENUM_COUNT];

	// 24
	AABB boundingBox;	
	Sphere boundingSphere;

	uint32_t __pad[4];

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


struct IModel
{
	virtual ~IModel() {}

	virtual const int addRef(void) X_ABSTRACT;
	virtual const int release(void) X_ABSTRACT;
	virtual const int forceRelease(void) X_ABSTRACT;

	virtual const char* getName(void) const X_ABSTRACT;
	virtual int32_t numLods(void) const X_ABSTRACT;
	virtual int32_t numBones(void) const X_ABSTRACT;
	virtual int32_t numBlankBones(void) const X_ABSTRACT;
	virtual int32_t numMeshTotal(void) const X_ABSTRACT;
	virtual int32_t numVerts(size_t lodIdx) const X_ABSTRACT;
	virtual bool HasLods(void) const X_ABSTRACT;

	virtual const AABB& bounds(void) const X_ABSTRACT;
	virtual const AABB& bounds(size_t lodIdx) const X_ABSTRACT;

	virtual const Sphere& boundingSphere(size_t lodIdx) const X_ABSTRACT;

	// temp.
	virtual void Render(void) X_ABSTRACT;
	virtual void RenderBones(const Matrix44f& modelMax) X_ABSTRACT;
};

struct IModelManager
{
	virtual ~IModelManager(){}

	// returns null if not found, ref count unaffected
	virtual IModel* findModel(const char* ModelName) const X_ABSTRACT;
	// if material is found adds ref and returns, if not try's to load the material file.
	// if file can't be loaded or error it return the default material.
	virtual IModel* loadModel(const char* ModelName) X_ABSTRACT;

	virtual IModel* getDefaultModel(void) X_ABSTRACT;
};


struct ModelHeader // File header.
{
	uint8_t version;
	Flags8<ModelFlags> flags;
	uint8_t numBones;
	uint8_t numBlankBones;
	uint8_t numMesh;
	uint8_t numLod;
	uint16_t boneDataSize;	// size of the bone data (pos,angles,tree)

	core::dateTimeStampSmall modified; // 4

	uint32_t dataSize;				// excludes size of header.
	uint32_t subDataSize;			// mesh heads + meshdata + bone data(not names)
	uint16_t materialNameDataSize;	// size of material name block
	uint16_t tagNameDataSize;		// size of tag name block.

	// do i need this?
	// the lods store one for each mesh so kinda redundant.
	AABB boundingBox;

	LODHeader lodInfo[MODEL_MAX_LODS];

	// definitions in 3DEngine::ModelLoader.cpp
	bool isValid(void) const;
};


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
X_ENSURE_SIZE(ModelHeader, (sizeof(LODHeader)*MODEL_MAX_LODS) + 48);



X_NAMESPACE_END

#endif // !_X_ASSET_MODEL_H_
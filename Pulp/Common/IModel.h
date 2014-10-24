#pragma once


#ifndef _X_ASSET_MODEL_H_
#define _X_ASSET_MODEL_H_

#include <Util\FlagsMacros.h>
#include <Util\Pointer64.h>
#include <String\StackString.h>
#include <Math\XAabb.h>
#include <Math\XQuatCompressed.h>
#include <Math\XVecCompressed.h>
#include <Time\CompressedStamps.h>

#include <IMaterial.h>

X_NAMESPACE_BEGIN(model)

// The Model Foramts
//
//  File Ext: .model
//	Version: 6.0 (i keep adding features / improving :( )
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
//			Verts[AllMesh]
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
//	

#define X_MODEL_BONES_LOWER_CASE_NAMES 1
#define X_MODEL_MTL_LOWER_CASE_NAMES 1

static const uint32_t	 MODEL_VERSION = 8;
static const uint32_t	 MODEL_MAX_BONES = 255;
static const uint32_t	 MODEL_MAX_BONE_NAME_LENGTH = 64;
static const uint32_t	 MODEL_MAX_MESH = 64;
static const uint32_t	 MODEL_MAX_VERTS = (1 << 16) - 1;
static const uint32_t	 MODEL_MAX_FACES = MODEL_MAX_VERTS;
// humm might make this 8 (would be for faces, probs make it diffrent type of model, 
// so not all models support 8, only a select few.)
static const uint32_t	 MODEL_MAX_VERT_BINDS = 4; 
static const uint32_t	 MODEL_MAX_LODS = 4;
static const uint32_t	 MODEL_MAX_NAME_LENGTH = 60;
static const char*		 MODEL_FILE_EXTENSION = ".model";

// Flags:
// LOOSE: the model file is not packed, meaning it has a string table that needs processing.
// FULL_VERT: the vertex data is not compressed.
// STREAMS: vertex data is split into streams. (a stream info block is included)
// ANIMATED: model has bind data.
X_DECLARE_FLAGS8(ModelFlags)(LOOSE, FULL_VERT, STREAMS, ANIMATED);
X_DECLARE_FLAGS(MeshState)(SYS_MEMORY, VIDEO_MEMORY);





struct bindWeight
{
	bindWeight(float val) {
		compressed_ = static_cast<uint16_t>(val * 65535.f);
	}

	operator float() const {
		return static_cast<float>(compressed_) / 65535.0f;
	}

	X_INLINE float weight() const {
		return static_cast<float>(compressed_) / 65535.0f;
	}
private:
	uint16_t compressed_; // scaled
};

struct bindBone
{
	bindBone() {}
	bindBone(uint16_t id) : compressed_(id * 128) {}

	X_INLINE uint16_t index() const {
		return compressed_ / 128;
	}

	// used for sorting
	const int operator-(const bindBone& tag) const {
		return compressed_ - tag.compressed_;
	}
	bool operator==(const bindBone& tag) const {
		return tag.compressed_ == compressed_;
	}
private:
	uint16_t compressed_; // scaled
};


struct singleBind
{
	uint16_t jointIdx; // 16bit for padding.
	uint16_t numVerts;
	uint16_t numFaces;
	uint16_t faceOffset;
};

struct Vertex // aim to keep this 32 bytes. or 16.
{
	Vec3f		pos;				// 12
	Color8u		col;				// 4
	Vec2f		uv;					// 8

	// 12 bytes to play with.
	// leaves these in here untill i have a better use for them.
//	compressedVec3 normal;		// 4
//	compressedVec3 tangent;		// 4
//	compressedVec3 binormal;	// 4
}; // 32


// a lossless layout
struct VertexFull
{
	Vec3f			pos;					
	Vec4<uint8_t>	col;
	Vec2f			uv;
	Vec3f			normal;
	Vec3f			tangent;
	Vec3f			binormal;
	uint8_t			_pad[4];
}; // 64 bytes

typedef uint16_t Index;
typedef Vec3<Index> Face;


struct CompbindInfo
{
	CompbindInfo() {
		core::zero_object(CompBinds_);
	}

	uint16_t& operator[](int idx) {
		X_ASSERT(idx >= 0 && idx < MODEL_MAX_VERT_BINDS, "index out of range")(idx, MODEL_MAX_VERT_BINDS);
		return CompBinds_[idx]; 
	}
	const uint16_t operator[](int idx) const {
		X_ASSERT(idx >= 0 && idx < MODEL_MAX_VERT_BINDS, "index out of range")(idx, MODEL_MAX_VERT_BINDS);
		return CompBinds_[idx]; 
	}

	X_INLINE size_t dataSize(int idx) const {
		// size is (idx * (bindBone + bindWeight)) + bindBone
		return CompBinds_[idx] * ((idx * 4) + 2);
	}

	X_INLINE size_t dataSizeTotal(void) const {
		size_t size = 0; int i;
		for (i = 0; i < MODEL_MAX_VERT_BINDS; i++)
			size += dataSize(i);
		return size;
	}

private:
	uint16_t CompBinds_[MODEL_MAX_VERT_BINDS];
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
	}

	// sizes
	uint16_t numVerts;
	uint16_t numIndexes;
	uint16_t numBinds;	// used for solid binds.
	uint16_t _unused; // 8

	// offset into the LOD's streames.
	uint32_t startVertex;
	uint32_t startIndex; // 8

	// needs to be void, since we have diffrent vertex formats.
	core::Pointer64<void>		verts; // 8
	core::Pointer64<Index>		indexes; // 8

	// 16
	core::Pointer64<IMaterial> pMat;
	core::Pointer64<const char> materialName;

	/* cold fields down here, shizz that's not 
		accessed much. */

	// bind sizes.
	CompbindInfo CompBinds; // 8

	// 80 so far.
	// lots of spare pickle bytes to play with.
	// should i add bounds info for each submesh?
	AABB boundingBox;
	Sphere boundingSphere;
	

	// moved out since it takes lots of room.
	// i have moved names into the subdata.
	// and only store a pointer in this class now
	// which is set on load.
//	core::StackString<MTL_MATERIAL_MAX_LEN> material;

//	X_NO_COPY(SubMeshHeader);
	X_NO_ASSIGN(SubMeshHeader);
}; // 96



// a mesh is a single Vb / IB
// It can have many sub-meshes.
// for 3D Models it's limits are above.
// level models have diffrent limits. <IBsp.h>
// respect both when changing type sizes.
struct MeshHeader
{
	MeshHeader() :
	numSubMeshes(0),
	_blank(0)
	{}

	// 4
	uint16_t	numSubMeshes; // levels support 65k sub meshes
	uint16_t	_blank;

	// 8 total of all sub-meshes.
	uint32_t    numVerts;
	uint32_t	numIndexes;

	// 4 + 4
	Flags<MeshState> state;
	uint32_t __blank;

	// 8
	core::Pointer64<SubMeshHeader> subMeshHeads;

	// Version 5, pointer to meshes verts / faces.
	// all sub meshes / index verts are stored contiguous
	core::Pointer64<void>		verts; // 8
	core::Pointer64<Index>		indexes; // 8

	// 24
	AABB boundingBox;	

	X_NO_COPY(MeshHeader);
	X_NO_ASSIGN(MeshHeader);
};


// a lod is a mesh with a distance
struct LODHeader : public MeshHeader
{
	LODHeader() :
	lodDistance(0.f)
	{}

	float lodDistance;
};


class XModel // a loaded mesh
{
	friend class ModelLoader;
public:
	XModel() : pData_(nullptr) {

		pTagNames_ = nullptr;
		pTagTree_ = nullptr;
		pBoneAngles_ = nullptr;
		pBonePos_ = nullptr;
		pMeshHeads_ = nullptr;

		numLods_ = 0;
		numBones_ = 0;
		numBlankBones_ = 0;
		totalMeshNum_ = 0;
	}
	~XModel() {
	//	delete pData_; // TODO: temp
	//	X_DELETE(pData,g_)
	}

	X_INLINE const char* getName(void) const { return name_.c_str(); }
	X_INLINE int numLods(void) const { return numLods_; }
	X_INLINE int numBones(void) const { return numBones_; }
	X_INLINE int numBlankBones(void) const { return numBlankBones_; }
	X_INLINE int totalMeshNum(void) const { return totalMeshNum_; }
	X_INLINE bool hasLods(void) const { return numLods_ > 1; }
	
	X_INLINE const LODHeader& getLod(int idx) const {
		X_ASSERT(idx < numLods_, "invalid lod index")(numLods(), idx);
		return lodInfo_[idx]; 
	}

	X_INLINE const SubMeshHeader* getMeshHead(int idx) const {
		X_ASSERT(idx < totalMeshNum(), "invalid mesh index")(totalMeshNum(), idx);
		return &pMeshHeads_[idx];
	}

private:
	X_NO_COPY(XModel);
	X_NO_ASSIGN(XModel);

	core::StackString<MODEL_MAX_NAME_LENGTH> name_;

	LODHeader lodInfo_[MODEL_MAX_LODS];

	// runtime pointers.
	const uint16_t*			pTagNames_;
	const uint8_t*			pTagTree_;
	const XQuatCompressedf* pBoneAngles_;
	const Vec3f*			pBonePos_;
	// pointer to all the meshes headers for all lods
	// sotred in lod order.
	const SubMeshHeader*	pMeshHeads_;

	int numLods_;
	int numBones_;
	int numBlankBones_;
	int totalMeshNum_; // combined mesh count of all lods.

	const char* pData_;
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
//	AABB boundingBox;

	LODHeader lodInfo[MODEL_MAX_LODS];

	bool isValid(void) const;
};


// check sizes., what ever is using these structs
// needs to be compiling these as same size so they can be loaded correct.
// the runtime pointers are wrapped to make sure always 64bits.
// so models work in both 32/64bit engine.
// while also allows the tool making the file to be 32bit.
X_ENSURE_SIZE(compressedVec3, 4);
X_ENSURE_SIZE(singleBind, 8);

X_ENSURE_SIZE(Vertex, 24);
X_ENSURE_SIZE(Face, 6);

X_ENSURE_SIZE(SubMeshHeader, 96);
X_ENSURE_SIZE(MeshHeader, 0x48);
X_ENSURE_SIZE(LODHeader, 0x48 + 8);
X_ENSURE_SIZE(ModelHeader, (sizeof(LODHeader)*MODEL_MAX_LODS) + (48 - 24));



X_NAMESPACE_END

#endif // !_X_ASSET_MODEL_H_
#include "stdafx.h"
#include "XModel.h"

#include <Memory\MemCursor.h>

#include <IMaterial.h>
#include <IPhysics.h>

X_NAMESPACE_BEGIN(model)

XModel::XModel(core::string& name) :
    core::AssetBase(name, assetDb::AssetType::MODEL),
    inverseBones_(g_ModelLibArena)
{
    id_ = 0;
    status_ = core::LoadStatus::NotLoaded;
    pTagNames_ = nullptr;
    pTagTree_ = nullptr;
    pBoneAngles_ = nullptr;
    pBonePos_ = nullptr;
    pMeshHeads_ = nullptr;
    pHdr_ = nullptr;
}

XModel::~XModel()
{
}

//
// Loads a model into a XModel
//
//	the data is:
//
//	||||| HEADER |||||

//  ||||| tag name IDX |||||
//  ||||| Bone Tree    |||||
//  ||||| Bone angles  |||||
//  ||||| Bone pos     |||||
//
//
//  ||||| All MeshHeaders   |||||
//  ||||| All the verts   |||||
//  ||||| All the faces   |||||
//
// I want to basically just check the header.
// then be able to calculate the total buffer size.
// load it and set the data pointers.
//

bool XModel::processData(core::UniquePointer<char[]> data, uint32_t dataSize, engine::IMaterialManager* pMatMan)
{
    if (dataSize < sizeof(ModelHeader)) {
        return false;
    }

    ModelHeader& hdr = *reinterpret_cast<ModelHeader*>(data.get());

    if (!hdr.isValid()) {
        X_ERROR("Model", "\"%s\" model header is invalid", name_.c_str());
        return false;
    }

    if (dataSize != (hdr.dataSize + sizeof(hdr))) {
        X_ERROR("Model", "\"%s\" incompleted data", name_.c_str());
        return false;
    }

    char* pDataBegin = (data.get() + sizeof(hdr));

    core::MemCursor mat_name_cursor(pDataBegin, hdr.materialNameDataSize);
    core::MemCursor tag_name_cursor(mat_name_cursor.end(), hdr.tagNameDataSize);
    core::MemCursor bone_data_cursor(tag_name_cursor.end(), hdr.boneDataSize);
    core::MemCursor phys_data_cursor(bone_data_cursor.end(), hdr.physDataSize);
    core::MemCursor hitbox_data_cursor(phys_data_cursor.end(), hdr.hitboxDataBlocks * 64);
    core::MemCursor cursor(hitbox_data_cursor.end(), hdr.meshDataSize);

    const size_t numBone = hdr.numBones;

    pTagNames_ = bone_data_cursor.postSeekPtr<uint16_t>(numBone);
    pTagTree_ = bone_data_cursor.postSeekPtr<uint8_t>(numBone);
    pBoneAngles_ = bone_data_cursor.postSeekPtr<XQuatCompressedf>(numBone);
    pBonePos_ = bone_data_cursor.postSeekPtr<Vec3f>(numBone);
    // relative data, maybe temp.
    pBoneAnglesRel_ = bone_data_cursor.postSeekPtr<XQuatCompressedf>(numBone);
    pBonePosRel_ = bone_data_cursor.postSeekPtr<Vec3f>(numBone);

    X_ASSERT(bone_data_cursor.isEof(), "Load error")
    (bone_data_cursor.numBytesRemaning());

    if (hdr.flags.IsSet(ModelFlag::PHYS_DATA)) {
        // nothing todo.
        // we use it later.
    }

    if (hdr.flags.IsSet(ModelFlag::HITBOX)) {
        X_ASSERT_NOT_IMPLEMENTED();
    }

    /*
	These streams are padded to 16byte align so we must seek past the
	the padding bytes manually.
	*/
    auto seekCursorToPad = [&cursor]() {
        // when the value is aligned we must work how much to seek by.
        // we do this by taking a coon to market.
        const char* pCur = cursor.getPtr<const char>();
        const char* pAligned = core::pointerUtil::AlignTop(pCur, 16);

        const size_t diff = union_cast<size_t>(pAligned - pCur);
        cursor.seekBytes(diff);
    };

    seekCursorToPad();

    pMeshHeads_ = cursor.getPtr<SubMeshHeader>();

    int32_t i;

    // we now have the mesh headers.
    for (i = 0; i < hdr.numLod; i++) {
        LODHeader& lod = hdr.lodInfo[i];

        lod.subMeshHeads = cursor.postSeekPtr<SubMeshHeader>(lod.numSubMeshes);
    }

    // ok we now need to set vert and face pointers.
    // for each Lod
    for (i = 0; i < hdr.numLod; i++) {
        LODHeader& lod = hdr.lodInfo[i];
        // we have 3 blocks of data.
        // Verts, Faces, Binddata
        SubMeshHeader* meshHeads = lod.subMeshHeads;

        seekCursorToPad();

        // verts (always provided)
        lod.streams[VertexStream::VERT] = cursor.postSeekPtr<Vertex>(lod.numVerts);
        X_ASSERT_ALIGNMENT(lod.streams[VertexStream::VERT].as<uintptr_t>(), 16, 0);
        seekCursorToPad();

        // color
        if (lod.streamsFlag.IsSet(StreamType::COLOR)) {
            lod.streams[VertexStream::COLOR] = cursor.postSeekPtr<VertexColor>(lod.numVerts);
            X_ASSERT_ALIGNMENT(lod.streams[VertexStream::COLOR].as<uintptr_t>(), 16, 0);
            seekCursorToPad();
        }

        // normals
        if (lod.streamsFlag.IsSet(StreamType::NORMALS)) {
            lod.streams[VertexStream::NORMALS] = cursor.postSeekPtr<VertexNormal>(lod.numVerts);
            X_ASSERT_ALIGNMENT(lod.streams[VertexStream::NORMALS].as<uintptr_t>(), 16, 0);
            seekCursorToPad();
        }

        // tangents bi-normals
        if (lod.streamsFlag.IsSet(StreamType::TANGENT_BI)) {
            lod.streams[VertexStream::TANGENT_BI] = cursor.postSeekPtr<VertexTangentBi>(lod.numVerts);
            X_ASSERT_ALIGNMENT(lod.streams[VertexStream::TANGENT_BI].as<uintptr_t>(), 16, 0);
            seekCursorToPad();
        }

        // now set the sub mesh pointers.
        {
            for (uint16_t x = 0; x < lod.numSubMeshes; x++) {
                SubMeshHeader& mesh = meshHeads[x];
                // when model is created the submesh streams have the byte offsets set.
                // so that we can just add the base address to fix them up.
                mesh.streams[VertexStream::VERT] += lod.streams[VertexStream::VERT];
                mesh.streams[VertexStream::COLOR] += lod.streams[VertexStream::COLOR];
                mesh.streams[VertexStream::NORMALS] += lod.streams[VertexStream::NORMALS];
                mesh.streams[VertexStream::TANGENT_BI] += lod.streams[VertexStream::TANGENT_BI];
            }
        }

        // indexes
        for (uint16_t x = 0; x < lod.numSubMeshes; x++) {
            SubMeshHeader& mesh = meshHeads[x];
            mesh.indexes = cursor.postSeekPtr<Index>(mesh.numFaces * 3);
        }

        // BindData
        if (hdr.flags.IsSet(ModelFlag::ANIMATED)) {
            lod.streams[VertexStream::HWSKIN] = cursor.getPtr<void>();

            for (uint16_t x = 0; x < lod.numSubMeshes; x++) {
                SubMeshHeader& mesh = meshHeads[x];

                size_t sizeSimple = mesh.numBinds * sizeof(simpleBind);
                size_t sizeComplex = mesh.CompBinds.dataSizeTotal();

                if (sizeSimple || sizeComplex) {
                    mesh.streams[VertexStream::HWSKIN] = cursor.getPtr<void>();

                    cursor.seekBytes(sizeSimple + sizeComplex);
                }
            }
        }

        // index 0 is always valid, since a valid lod must
        // have a mesh.
        lod.indexes = meshHeads[0].indexes;

        X_ASSERT_ALIGNMENT(lod.indexes.as<uintptr_t>(), 16, 0);
    }

    X_ASSERT(cursor.isEof(), "Load error")
    (cursor.numBytesRemaning());

    // even tho the names are stored at the top of the file we process them now.
    // so that i can set name pointers in the MeshHeaders for convience :D !

    // Material name list null-term
    {
        core::StackString<engine::MTL_MATERIAL_MAX_LEN> name;
        for (i = 0; i < hdr.numMesh; i++) {
            SubMeshHeader* pMesh = const_cast<SubMeshHeader*>(&pMeshHeads_[i]);
            // set the pointer.
            pMesh->materialName = mat_name_cursor.getPtr<const char>();

            while (!mat_name_cursor.isEof() && mat_name_cursor.get<char>() != '\0') {
                name.append(mat_name_cursor.getSeek<char>(), 1);
            }

            if (!mat_name_cursor.isEof()) {
                ++mat_name_cursor;
            }

            name.clear();
        }
    }

    // Tag name list null-term
    if (hdr.flags.IsSet(ModelFlags::LOOSE) && hdr.tagNameDataSize > 0) {
        core::StackString<MODEL_MAX_BONE_NAME_LENGTH> name;

        for (i = 0; i < hdr.numBones; i++) {
            ((uint16_t*)pTagNames_)[i] = safe_static_cast<uint16>(tag_name_cursor.getPtr<char>() - pDataBegin);

            while (!tag_name_cursor.isEof() && tag_name_cursor.get<char>() != '\0') {
                name.append(tag_name_cursor.getSeek<char>(), 1);
            }

            // TODO: add these names to the string table.
            if (!tag_name_cursor.isEof()) {
                ++tag_name_cursor;
            }

            name.clear();
        }
    }

    X_ASSERT(mat_name_cursor.isEof(), "Load error")
    (mat_name_cursor.numBytesRemaning());
    X_ASSERT(tag_name_cursor.isEof(), "Load error")
    (tag_name_cursor.numBytesRemaning());

    // load the materials.
    for (i = 0; i < hdr.numMesh; i++) {
        SubMeshHeader& mesh = const_cast<SubMeshHeader&>(pMeshHeads_[i]);
        mesh.pMat = pMatMan->loadMaterial(mesh.materialName);
    }

    // create the inverse bone matrix.
    // technically we don't need to make this unless we are animating it.
    inverseBones_.resize(hdr.numBones);
    for (int32_t i = 0; i < hdr.numBones; i++) {
        auto& angle = getBoneAngle(i);
        auto& pos = getBonePos(i);
        auto quat = angle.asQuat();

        inverseBones_[i] = quat.toMatrix33();
        inverseBones_[i].setTranslate(pos);
        inverseBones_[i].invert();
    }

    data_ = std::move(data);
    pHdr_ = reinterpret_cast<ModelHeader*>(data_.ptr());
    return true;
}

void XModel::addPhysToActor(physics::ActorHandle actor)
{
    X_ASSERT(pHdr_->flags.IsSet(ModelFlag::PHYS_DATA), "no phys data")
    ();

    // we need the col header
    core::MemCursor phys_data_cursor(data_.get() + sizeof(ModelHeader) + pHdr_->materialNameDataSize + pHdr_->tagNameDataSize + pHdr_->boneDataSize, pHdr_->physDataSize);

    const CollisionInfoHdr& hdr = *phys_data_cursor.getSeekPtr<CollisionInfoHdr>();

    size_t total = hdr.totalShapes();
    phys_data_cursor.seekBytes(total - 1);

    auto* pPhys = gEnv->pPhysics;
    for (uint8_t t = 0; t < hdr.shapeCounts[ColMeshType::BOX]; t++) {
        AABB* pAABB = phys_data_cursor.getSeekPtr<AABB>();
        const Vec3f localPose = pAABB->center();
        pPhys->addBox(actor, *pAABB, localPose);
    }

    for (uint8_t t = 0; t < hdr.shapeCounts[ColMeshType::SPHERE]; t++) {
        Sphere* pSphere = phys_data_cursor.getSeekPtr<Sphere>();
        pPhys->addSphere(actor, pSphere->radius(), pSphere->center());
    }

    for (uint8_t t = 0; t < hdr.shapeCounts[ColMeshType::CONVEX]; t++) {
        CollisionConvexHdr* pConvexHdr = phys_data_cursor.getSeekPtr<CollisionConvexHdr>();

        if (pHdr_->flags.IsSet(ModelFlag::PHYS_BAKED)) {
            size_t len = pConvexHdr->dataSize;
            const uint8_t* pData = phys_data_cursor.postSeekPtr<uint8_t>(len);

            auto convexMeshHandle = pPhys->createConvexMesh(pData, len);
            pPhys->addConvexMesh(actor, convexMeshHandle);
        }
        else {
            X_ASSERT_NOT_IMPLEMENTED();
        }
    }
}

X_NAMESPACE_END
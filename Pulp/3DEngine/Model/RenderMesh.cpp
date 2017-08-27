#include "stdafx.h"
#include "RenderMesh.h"


#include <IModel.h>
#include <IRender.h>
#include <IShader.h>


X_NAMESPACE_BEGIN(model)


namespace
{

	// these are here till i can think of better place to put them.
	static uint32_t vertexFormatStride[render::shader::VertexFormat::ENUM_COUNT] =
	{
		sizeof(Vertex_P3F_T3F),					// P3F_T3F

		sizeof(Vertex_P3F_T2S),					// P3F_T2S
		sizeof(Vertex_P3F_T2S_C4B),				// P3F_T2S_C4B
		sizeof(Vertex_P3F_T2S_C4B_N3F),			// P3F_T2S_C4B_N3F
		sizeof(Vertex_P3F_T2S_C4B_N3F_TB3F),	// P3F_T2S_C4B_N3F_TB3F

		sizeof(Vertex_P3F_T2S_C4B_N10),			// P3F_T2S_C4B_N10
		sizeof(Vertex_P3F_T2S_C4B_N10_TB10),	// P3F_T2S_C4B_N10_TB10

		sizeof(Vertex_P3F_T2F_C4B),				// P3F_T2F_C4B

		sizeof(Vertex_P3F_T4F_C4B_N3F),			// P3F_T4F_C4B_N3F
	};


	static uint32_t vertexSteamStride[VertexStream::ENUM_COUNT][render::shader::VertexFormat::ENUM_COUNT] =
	{
		// base vert stream
		{
			sizeof(Vertex_P3F_T3F), // large uv's
			sizeof(Vertex_P3F_T2S),
			sizeof(Vertex_P3F_T2S),
			sizeof(Vertex_P3F_T2S),
			sizeof(Vertex_P3F_T2S),
			sizeof(Vertex_P3F_T2S),
			sizeof(Vertex_P3F_T2S),
			sizeof(Vertex_P3F_T2S),
			sizeof(Vertex_P3F_T4F),
		},
		// color
		{
			0, // no color
			0,
			sizeof(Color8u),
			sizeof(Color8u),
			sizeof(Color8u),
			sizeof(Color8u),
			sizeof(Color8u),
			sizeof(Color8u),
			sizeof(Color8u),
		},
		// Normals
		{
			0,
			0,
			0,
			sizeof(Vec3f),
			sizeof(Vec3f),
			sizeof(compressedVec3),
			sizeof(compressedVec3),
			0,
			sizeof(Vec3f)
		},
		// Tangent Binormals
		{
			0,
			0,
			0,
			0,
			sizeof(Vertex_Tangents_BiNorm),
			0,
			sizeof(Vertex_Tangents_BiNorm),
			0,
			0
		},
	};


} // namespace

XRenderMesh::SizeInfo::SizeInfo()
{
	core::zero_this(this);
}

// -------------------------------------------

XRenderMesh::XRenderMesh()
{
	vertexStreams_.fill(render::INVALID_BUF_HANLDE);
	indexStream_ = render::INVALID_BUF_HANLDE;
}

bool XRenderMesh::canRender(void) const
{
	return indexStream_ != render::INVALID_BUF_HANLDE &&
		vertexStreams_[VertexStream::VERT] != render::INVALID_BUF_HANLDE;
}

bool XRenderMesh::createRenderBuffers(render::IRender* pRend, const MeshHeader& mesh, VertexFormat::Enum vertexFmt)
{
	X_ASSERT_NOT_NULL(pRend);

	const uint32_t numVerts = mesh.numVerts;
	const uint32_t baseVertStride = vertexSteamStride[VertexStream::VERT][vertexFmt];
	const uint32_t colorStride = vertexSteamStride[VertexStream::COLOR][vertexFmt];
	const uint32_t normalStride = vertexSteamStride[VertexStream::NORMALS][vertexFmt];
	const uint32_t tanBiStride = vertexSteamStride[VertexStream::TANGENT_BI][vertexFmt];

	indexStream_ = pRend->createIndexBuffer(sizeof(model::Index), mesh.numIndexes, mesh.indexes, render::BufUsage::IMMUTABLE);

	// do we always want to upload every stream also?
	// i think not.
	// on a per model bases we want to ommit certain streams.
	// since if we are not going to use them it's wasted vram.


	// we always carry vert?
	X_ASSERT(baseVertStride > 0, "Vertex stride of zero")(baseVertStride);
	vertexStreams_[VertexStream::VERT] = pRend->createVertexBuffer(baseVertStride, numVerts,
		mesh.streams[VertexStream::VERT], render::BufUsage::IMMUTABLE);

	if (colorStride > 0) {
		vertexStreams_[VertexStream::COLOR] = pRend->createVertexBuffer(colorStride, numVerts,
			mesh.streams[VertexStream::COLOR], render::BufUsage::IMMUTABLE);
	}
	if (normalStride > 0) {
		vertexStreams_[VertexStream::NORMALS] = pRend->createVertexBuffer(normalStride, numVerts,
			mesh.streams[VertexStream::NORMALS], render::BufUsage::IMMUTABLE);
	}
	if (tanBiStride > 0) {
		vertexStreams_[VertexStream::TANGENT_BI] = pRend->createVertexBuffer(tanBiStride, numVerts,
			mesh.streams[VertexStream::TANGENT_BI], render::BufUsage::IMMUTABLE);
	}

	return canRender();
}

bool XRenderMesh::createSkinningRenderBuffers(render::IRender* pRend, const MeshHeader& mesh)
{
	if (!mesh.flags.IsSet(MeshFlag::ANIMATED)) {
		X_WARNING("RenderMesh", "Can't create skinning buffers for none animated mesh");
		return false;
	}

	if (vertexStreams_[VertexStream::HWSKIN] != render::INVALID_BUF_HANLDE) {
		X_WARNING("RenderMesh", "Skinning stream is already valid");
		return false;
	}

	core::Array<Vertex_SkinData, core::ArrayAlignedAllocatorFixed<Vertex_SkinData, 16>> skinData(g_3dEngineArena);
	skinData.resize(mesh.numVerts);


	for (int32_t meshIdx = 0; meshIdx < mesh.numSubMeshes; meshIdx++)
	{
		const auto* pSubMesh = mesh.subMeshHeads.operator[](meshIdx);
		Vertex_SkinData* pMeshSkinData = &skinData[pSubMesh->startVertex];

		if (pSubMesh->numBinds)
		{
			const simpleBind* pSimpleBind = pSubMesh->streams[VertexStream::HWSKIN].as<simpleBind>();

			for (int32_t i = 0; i < pSubMesh->numBinds; i++)
			{
				auto& sb = pSimpleBind[i];

				Vertex_SkinData data;
				data.indexes[0] = safe_static_cast<uint8_t>(sb.jointIdx);
				data.weights[0] = std::numeric_limits<uint16_t>::max();

				for (int32_t v = 0; v < sb.numVerts; v++)
				{
					pMeshSkinData[sb.startVert + v] = data;
				}
			}
		}
		else if(pSubMesh->CompBinds.hasData())
		{
			X_ASSERT_NOT_IMPLEMENTED();
			
			const auto& cb = pSubMesh->CompBinds;

			const uint8_t* pData = pSubMesh->streams[VertexStream::HWSKIN].as<uint8_t>();

			if (cb[0])
			{
				int32_t numSingle = cb[0];

				const bindBone* pSingle = reinterpret_cast<const bindBone*>(pData);
				for (int32_t i = 0; i < numSingle; i++)
				{
					pMeshSkinData[i].indexes[0] = pSingle[i].getIndex();
					pMeshSkinData[i].weights[0] = std::numeric_limits<uint16_t>::max();
				}

				pData += cb.dataSize(0);
			}

			if (cb[1])
			{
				int32_t numDouble = cb[1];

				const doubleBind* pDouble = reinterpret_cast<const doubleBind*>(pData);
				for (int32_t i = 0; i < numDouble; i++, ++pMeshSkinData, ++pDouble)
				{
					uint16_t weight2 = pDouble->weight2;

					pMeshSkinData->indexes[0] = pDouble->bone1.getIndex();
					pMeshSkinData->indexes[1] = pDouble->bone2.getIndex();
					pMeshSkinData->weights[0] = std::numeric_limits<uint16_t>::max() - weight2;
					pMeshSkinData->weights[1] = weight2;
				}

				pData += cb.dataSize(1);
			}

			if (cb[2])
			{
				int32_t numTripple = cb[2];

				const trippleBind* pTripple = reinterpret_cast<const trippleBind*>(pData);
				for (int32_t i = 0; i < numTripple; i++, ++pMeshSkinData, ++pTripple)
				{
					uint16_t weight2 = pTripple->weight2;
					uint16_t weight3 = pTripple->weight3;

					pMeshSkinData->indexes[0] = pTripple->bone1.getIndex();
					pMeshSkinData->indexes[1] = pTripple->bone2.getIndex();
					pMeshSkinData->indexes[2] = pTripple->bone3.getIndex();

					pMeshSkinData->weights[0] = std::numeric_limits<uint16_t>::max() - (weight2 + weight3);
					pMeshSkinData->weights[1] = weight2;
					pMeshSkinData->weights[2] = weight3;
				}

				pData += cb.dataSize(2);
			}

			if (cb[3])
			{
				int32_t numQuad = cb[3];

				const quadBind* pQuad = reinterpret_cast<const quadBind*>(pData);
				for (int32_t i = 0; i < numQuad; i++, ++pMeshSkinData, ++pQuad)
				{
					uint16_t weight2 = pQuad->weight2;
					uint16_t weight3 = pQuad->weight3;
					uint16_t weight4 = pQuad->weight4;

					pMeshSkinData->indexes[0] = pQuad->bone1.getIndex();
					pMeshSkinData->indexes[1] = pQuad->bone2.getIndex();
					pMeshSkinData->indexes[2] = pQuad->bone3.getIndex();
					pMeshSkinData->indexes[3] = pQuad->bone4.getIndex();

					pMeshSkinData->weights[0] = std::numeric_limits<uint16_t>::max() - (weight2 + weight3 + weight4);
					pMeshSkinData->weights[1] = weight2;
					pMeshSkinData->weights[2] = weight3;
					pMeshSkinData->weights[3] = weight4;
				}

			}

		}

	}

	vertexStreams_[VertexStream::HWSKIN] = pRend->createVertexBuffer(sizeof(Vertex_SkinData), mesh.numVerts, skinData.data(), render::BufUsage::IMMUTABLE);

	return true;
}

void XRenderMesh::releaseRenderBuffers(render::IRender* pRend)
{
	X_ASSERT_NOT_NULL(pRend);

	if (indexStream_ != render::INVALID_BUF_HANLDE) {
		pRend->destoryIndexBuffer(indexStream_);
		indexStream_ = render::INVALID_BUF_HANLDE;
	}

	for (size_t i = 0; i< VertexStream::ENUM_COUNT; i++) {
		if (vertexStreams_[i] != render::INVALID_BUF_HANLDE) {
			pRend->destoryVertexBuffer(vertexStreams_[i]);
			vertexStreams_[i] = render::INVALID_BUF_HANLDE;
		}
	}
}

bool XRenderMesh::ensureRenderStream(render::IRender* pRend, const MeshHeader& mesh, VertexFormat::Enum vertexFmt, VertexStream::Enum stream)
{
	if (vertexStreams_[stream] != render::INVALID_BUF_HANLDE) {
		return true;
	}

	// we need to make it :|
	// the model should tell use what the vertex format of the streams is.
	// so that we know the strides etc..

	const uint32_t numVerts = mesh.numVerts;
	const uint32_t stride = vertexSteamStride[stream][vertexFmt];

	if (stride == 0) {
		X_ERROR("RenderMesh", "Requested a device buffer for a vertex stream that is not present: \"%s\"", VertexStream::ToString(stream));
		return false;
	}

	// make the stream :D !
	vertexStreams_[stream] = pRend->createVertexBuffer(stride, numVerts, mesh.streams[stream], render::BufUsage::IMMUTABLE);
	return true;
}

void XRenderMesh::releaseVertexBuffer(render::IRender* pRend, VertexStream::Enum stream)
{
	if (vertexStreams_[stream] != render::INVALID_BUF_HANLDE) {
		pRend->destoryVertexBuffer(vertexStreams_[stream]);
		vertexStreams_[stream] = render::INVALID_BUF_HANLDE;
	}
}

void XRenderMesh::releaseIndexBuffer(render::IRender* pRend)
{
	if (indexStream_ != render::INVALID_BUF_HANLDE) {
		pRend->destoryIndexBuffer(indexStream_);
		indexStream_ = render::INVALID_BUF_HANLDE;
	}
}

XRenderMesh::SizeInfo XRenderMesh::memoryUsage(render::IRender* pRend) const
{
	X_ASSERT_NOT_NULL(pRend);

	SizeInfo info;
	int32_t size, deviceSize;

	if (indexStream_ != render::INVALID_BUF_HANLDE) {
		pRend->getVertexBufferSize(indexStream_, &size, &deviceSize);
	
		info.size += size;
		info.deviceSize += deviceSize;
	}

	for (size_t i = 0; i< VertexStream::ENUM_COUNT; i++) {
		if (vertexStreams_[i] != render::INVALID_BUF_HANLDE) {
			pRend->getVertexBufferSize(vertexStreams_[i], &size, &deviceSize);
			
			info.size += size;
			info.deviceSize += deviceSize;
		}
	}

	return info;
}



X_NAMESPACE_END
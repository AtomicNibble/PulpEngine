#include "stdafx.h"
#include "XRenderMesh.h"

#include "Dx10Render.h"

#include "DeviceManager\VidMemManager.h"

#include "../Textures/XTexture.h"

X_NAMESPACE_BEGIN(model)

XMeshDevBuf::XMeshDevBuf() :
    BufId(render::VidMemManager::null_id),
    stride(0)
{
}

bool XMeshDevBuf::isValid(void) const
{
    return BufId != render::VidMemManager::null_id;
}

XRenderMesh::XRenderMesh()
{
    pMesh_ = nullptr;
    vertexFmt_ = (shader::VertexFormat::Enum)-1;

    indexStream_.BufId = render::VidMemManager::null_id;
}

XRenderMesh::XRenderMesh(const model::MeshHeader* pMesh, shader::VertexFormat::Enum fmt,
    const char* pName)
{
    X_ASSERT_NOT_NULL(pMesh);
    X_ASSERT_NOT_NULL(pName);

    name_ = pName;
    pMesh_ = pMesh;
    vertexFmt_ = fmt;

    indexStream_.BufId = render::VidMemManager::null_id;
}

XRenderMesh::~XRenderMesh()
{
}

// returns false if no Video memory.
bool XRenderMesh::canRender(void)
{
    using namespace render;

    return indexStream_.BufId != VidMemManager::null_id && vertexStreams_[VertexStream::VERT].BufId != VidMemManager::null_id;
}

// DX11XRender::vertexFormatStride[vertexFmt_],

bool XRenderMesh::uploadToGpu(void)
{
    X_ASSERT_NOT_NULL(pMesh_);
    X_ASSERT((int32)vertexFmt_ != -1, "vertex format has not been set")
    (vertexFmt_);
    using namespace render;

    uint32_t ibSize = pMesh_->numIndexes * sizeof(model::Index);
    uint32_t numVerts = pMesh_->numVerts;

    uint32_t baseVertStride = DX11XRender::vertexSteamStride[VertexStream::VERT][vertexFmt_];
    uint32_t ColorStride = DX11XRender::vertexSteamStride[VertexStream::COLOR][vertexFmt_];
    uint32_t normalStride = DX11XRender::vertexSteamStride[VertexStream::NORMALS][vertexFmt_];
    uint32_t tanBiStride = DX11XRender::vertexSteamStride[VertexStream::TANGENT_BI][vertexFmt_];

    indexStream_.BufId = g_Dx11D3D.VidMemMng()->CreateIB(ibSize, pMesh_->indexes);

    if (baseVertStride > 0) {
        vertexStreams_[VertexStream::VERT].BufId = g_Dx11D3D.VidMemMng()->CreateVB(baseVertStride * numVerts,
            pMesh_->streams[VertexStream::VERT]);

        vertexStreams_[VertexStream::VERT].stride = baseVertStride;
    }
    if (ColorStride > 0) {
        vertexStreams_[VertexStream::COLOR].BufId = g_Dx11D3D.VidMemMng()->CreateVB(ColorStride * numVerts,
            pMesh_->streams[VertexStream::COLOR]);

        vertexStreams_[VertexStream::COLOR].stride = ColorStride;
    }
    if (normalStride > 0) {
        vertexStreams_[VertexStream::NORMALS].BufId = g_Dx11D3D.VidMemMng()->CreateVB(normalStride * numVerts,
            pMesh_->streams[VertexStream::NORMALS]);

        vertexStreams_[VertexStream::NORMALS].stride = normalStride;
    }
    if (tanBiStride > 0) {
        vertexStreams_[VertexStream::TANGENT_BI].BufId = g_Dx11D3D.VidMemMng()->CreateVB(tanBiStride * numVerts,
            pMesh_->streams[VertexStream::TANGENT_BI]);

        vertexStreams_[VertexStream::TANGENT_BI].stride = tanBiStride;
    }

    return canRender();
}

bool XRenderMesh::render(void)
{
    X_PROFILE_BEGIN("RenderMesh", core::profiler::SubSys::RENDER);

    using namespace render;

    if (!canRender())
        return false;

    if (vertexFmt_ == shader::VertexFormat::P3F_T4F_C4B_N3F) {
        if (!g_Dx11D3D.SetWorldShader()) {
            return false;
        }
    }
    else {
        if (!g_Dx11D3D.SetModelShader(vertexFmt_)) {
            return false;
        }
    }

    g_Dx11D3D.FX_SetVertexDeclaration(vertexFmt_, true);
    g_Dx11D3D.FX_SetIStream(indexStream_.BufId);

    XMeshDevBuf& verts = vertexStreams_[VertexStream::VERT];
    XMeshDevBuf& color = vertexStreams_[VertexStream::COLOR];
    XMeshDevBuf& normals = vertexStreams_[VertexStream::NORMALS];
    XMeshDevBuf& tanBi = vertexStreams_[VertexStream::TANGENT_BI];

    if (!verts.isValid()) {
        return false;
    }

    g_Dx11D3D.FX_SetVStream(
        verts.BufId,
        VertexStream::VERT,
        verts.stride,
        0);

    if (color.isValid()) {
        g_Dx11D3D.FX_SetVStream(
            color.BufId,
            VertexStream::COLOR,
            color.stride,
            0);
    }
    if (normals.isValid()) {
        g_Dx11D3D.FX_SetVStream(
            normals.BufId,
            VertexStream::NORMALS,
            normals.stride,
            0);
    }
    if (tanBi.isValid()) {
        g_Dx11D3D.FX_SetVStream(
            tanBi.BufId,
            VertexStream::TANGENT_BI,
            tanBi.stride,
            0);
    }

    uint32_t i, num;
    num = pMesh_->numSubMeshes;

    bool drawNoneOpaque = false;

    for (i = 0; i < num; i++) {
        const model::SubMeshHeader* mesh = pMesh_->subMeshHeads[i];

        engine::IMaterial* pMat = mesh->pMat;
        shader::XShaderItem& shaderItem = pMat->getShaderItem();

        engine::MaterialFlags flags = pMat->getFlags();
        if (flags.IsSet(engine::MaterialFlag::NODRAW)) {
            continue;
        }

        if (pMat->getCoverage() != engine::MaterialCoverage::OPAQUE) {
            drawNoneOpaque = true;
            continue;
        }

        if (shaderItem.pResources_) {
            shader::XTextureResource* pTextRes = shaderItem.pResources_->getTexture(shader::ShaderTextureIdx::DIFFUSE);
            if (pTextRes) {
                texture::TexID id = pTextRes->pITex->getTexID();
                texture::XTexture::applyFromId(0, id, 0);
            }
            pTextRes = shaderItem.pResources_->getTexture(shader::ShaderTextureIdx::BUMP);
            if (pTextRes) {
                texture::TexID id = pTextRes->pITex->getTexID();
                texture::XTexture::applyFromId(1, id, 0);
            }
        }

        g_Dx11D3D.FX_DrawIndexPrimitive(
            PrimitiveType::TriangleList,
            mesh->numIndexes,
            mesh->startIndex,
            mesh->startVertex);
    }

    if (drawNoneOpaque) {
        for (i = 0; i < num; i++) {
            const model::SubMeshHeader* mesh = pMesh_->subMeshHeads[i];
            engine::IMaterial* pMat = mesh->pMat;

            if (pMat->getCoverage() == engine::MaterialCoverage::OPAQUE) {
                continue;
            }

            engine::MaterialFlags flags = pMat->getFlags();
            if (flags.IsSet(engine::MaterialFlag::NODRAW)) {
                continue;
            }

            shader::XShaderItem& shaderItem = pMat->getShaderItem();
            if (shaderItem.pResources_) {
                shader::XTextureResource* pTextRes = shaderItem.pResources_->getTexture(shader::ShaderTextureIdx::DIFFUSE);
                if (pTextRes) {
                    texture::TexID id = pTextRes->pITex->getTexID();
                    texture::XTexture::applyFromId(0, id, 0);
                }
                pTextRes = shaderItem.pResources_->getTexture(shader::ShaderTextureIdx::BUMP);
                if (pTextRes) {
                    texture::TexID id = pTextRes->pITex->getTexID();
                    texture::XTexture::applyFromId(1, id, 0);
                }
            }

            g_Dx11D3D.FX_DrawIndexPrimitive(
                PrimitiveType::TriangleList,
                mesh->numIndexes,
                mesh->startIndex,
                mesh->startVertex);
        }
    }

    return true;
}

// genral Info
const char* XRenderMesh::getName(void) const
{
    return name_.c_str();
}

int XRenderMesh::getNumVerts(void) const
{
    return pMesh_->numVerts;
}

int XRenderMesh::getNumIndexes(void) const
{
    return pMesh_->numIndexes;
}

int XRenderMesh::getNumSubMesh(void) const
{
    return pMesh_->numSubMeshes;
}

// Mem Info
int XRenderMesh::MemoryUsageTotal(void) const
{
    return MemoryUsageSys() + MemoryUsageVideo();
}

int XRenderMesh::MemoryUsageSys(void) const
{
    return 0;
}

int XRenderMesh::MemoryUsageVideo(void) const
{
    return 0;
}

void XRenderMesh::freeVB(VertexStream::Enum stream) // VertexBuffer
{
    if (vertexStreams_[stream].isValid()) {
    }
}

void XRenderMesh::freeIB(void) // Index Buffer
{
    X_ASSERT_NOT_IMPLEMENTED();
}

void XRenderMesh::freeVideoMem(bool restoreSys)
{
    X_UNUSED(restoreSys);
    X_ASSERT_NOT_IMPLEMENTED();
}

void XRenderMesh::freeSystemMem(void)
{
    X_ASSERT_NOT_IMPLEMENTED();
}

X_NAMESPACE_END
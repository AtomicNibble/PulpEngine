#include "stdafx.h"
#include "XRender.h"

#include <String\HumanSize.h>
#include <Util\LastError.h>
#include <ICore.h>

#include <ITexture.h>
#include <IConsole.h>

#include "Textures\XTexture.h"
#include "Model\XRenderMesh.h"

#if X_PLATFORM_WIN32 == 1
X_LINK_LIB("D3D11.lib")
X_LINK_LIB("D3Dx10.lib")
X_LINK_LIB("dxgi.lib")
#endif

X_NAMESPACE_BEGIN(render)

using namespace texture;

XRender* gRenDev = nullptr;
ID3D11RenderTargetView* backbuffer = nullptr; // global declaration

// this needs to be in order 1:1 mapping with enums
uint32_t XRender::vertexFormatStride[shader::VertexFormat::Num] = {
    sizeof(Vertex_P3F_T3F), // P3F_T3F

    sizeof(Vertex_P3F_T2S),              // P3F_T2S
    sizeof(Vertex_P3F_T2S_C4B),          // P3F_T2S_C4B
    sizeof(Vertex_P3F_T2S_C4B_N3F),      // P3F_T2S_C4B_N3F
    sizeof(Vertex_P3F_T2S_C4B_N3F_TB3F), // P3F_T2S_C4B_N3F_TB3F

    sizeof(Vertex_P3F_T2S_C4B_N10),      // P3F_T2S_C4B_N10
    sizeof(Vertex_P3F_T2S_C4B_N10_TB10), // P3F_T2S_C4B_N10_TB10

    sizeof(Vertex_P3F_T2F_C4B), // P3F_T2F_C4B

    sizeof(Vertex_P3F_T4F_C4B_N3F), // P3F_T4F_C4B_N3F
};

uint32_t XRender::vertexSteamStride[VertexStream::ENUM_COUNT][shader::VertexFormat::Num] = {
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
        sizeof(Vec3f)},
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
        0},
};

XRender::XRender() :
    fontIdx_(0),
    pDefaultFont_(nullptr),
    RenderResources_(g_rendererArena),
    //	textDrawList_(g_rendererArena)
    pTextDrawList_(nullptr)
{
    // try make sure that the array above is valid.
#if X_DEBUG

    for (int i = 0; i < shader::VertexFormat::Num; i++) {
        X_ASSERT(vertexFormatStride[i] > 0, "invalid vertex stride info")
        ();
    }

#endif // !X_DEBUG
}

XRender::~XRender()
{
    // THIS function is not called at shutdown.
    // render object is a global in the dll.
    //
    // Place code in 'Shutdown' instead.
}

void XRender::SetArenas(core::MemoryArenaBase* arena)
{
    X_ASSERT_NOT_NULL(arena);

    RenderResources_.setArena(arena);
    //	textDrawList_.setArena(arena);
}

bool XRender::Init(HWND hWnd, uint32_t width, uint32_t height)
{
    X_UNUSED(hWnd);

    ViewPort_.set(width, height);
    ViewPort_.setZ(0.f, 1.f);

    pTextDrawList_ = X_NEW(XTextDrawList, g_rendererArena, "RenderTextDrawList")(g_rendererArena);
    pTextDrawList_->setCapacity(500 * 512);

    vidMemMng_.StartUp();

    if (gEnv->pFont) {
        pDefaultFont_ = gEnv->pFont->GetFont("default");
    }

    RegisterVars();
    return true;
}

void XRender::RegisterVars(void)
{
    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->pConsole);

    ADD_CVAR_REF_COL_NO_NAME(r_clear_color, Color(0.057f, 0.221f, 0.400f),
        core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "Clear color");
}

void XRender::ShutDown(void)
{
    X_LOG0("render", "Shutting Down");

    freeResources();
}

void XRender::freeResources(void)
{
    texture::XTexture::shutDown();

    ShaderMan_.Shutdown();

    vidMemMng_.ShutDown();

    pTextDrawList_->free();
    X_DELETE_AND_NULL(pTextDrawList_, g_rendererArena);
}

void XRender::RenderBegin(void)
{
}

void XRender::RenderEnd(void)
{
    FlushTextBuffer();
}

// ViewPort

void XRender::GetViewport(int* left, int* top, int* right, int* bottom)
{
    X_ASSERT_NOT_NULL(left);
    X_ASSERT_NOT_NULL(top);
    X_ASSERT_NOT_NULL(right);
    X_ASSERT_NOT_NULL(bottom);

    const Recti& rect = ViewPort_.getRect();

    *left = rect.x1;
    *top = rect.y1;
    *right = rect.x2;
    *bottom = rect.y2;
}

void XRender::SetViewport(int left, int top, int right, int bottom)
{
    ViewPort_.set(left, top, right, bottom);
}

void XRender::GetViewport(Recti& rect)
{
    rect = ViewPort_.getRect();
}

void XRender::SetViewport(const Recti& rect)
{
    ViewPort_.getRect() = rect;
}

// ~ViewPort

// Textures
texture::ITexture* XRender::LoadTexture(const char* path, texture::TextureFlags flags)
{
    X_ASSERT_NOT_NULL(path);

    XTexture* pText = XTexture::FromName(path, flags);

    return pText;
}

// ~Textures

// Font

int XRender::FontCreateTexture(const Vec2i& size, BYTE* pData,
    Texturefmt::Enum textureFmt, bool genMips)
{
    if (!pData)
        return -1;

    TextureFlags Flags = TextureFlags::TEX_FONT | TextureFlags::DONT_STREAM | TextureFlags::DONT_RESIZE;

    if (genMips)
        Flags.Set(TextureFlags::FORCE_MIPS);

    core::StackString<64> name;
    name.appendFmt("$font_texure_%i", fontIdx_++);

    XTexture* tex = XTexture::Create2DTexture(name.c_str(), size,
        1, Flags, pData, textureFmt);

    return tex->getID();
}

void XRender::DrawStringW(font::IXFont_RenderProxy* pFont, const Vec3f& pos,
    const wchar_t* pStr, const font::TextDrawContext& ctx) const
{
    pFont->RenderCallback(pos, pStr, ctx);
}

// ~Font

// Model

model::IRenderMesh* XRender::createRenderMesh(void)
{
    model::XRenderMesh* pMesh = X_NEW(
        model::XRenderMesh, g_rendererArena,
        "RenderMesh")();

    return pMesh;
}

model::IRenderMesh* XRender::createRenderMesh(const model::MeshHeader* pMesh,
    shader::VertexFormat::Enum fmt, const char* name)
{
    model::XRenderMesh* pRenMesh = X_NEW(
        model::XRenderMesh, g_rendererArena,
        "RenderMesh")(pMesh, fmt, name);

    return pRenMesh;
}

void XRender::freeRenderMesh(model::IRenderMesh* pMesh)
{
    X_ASSERT_NOT_NULL(pMesh);

    core::SafeRelease(pMesh);
}

// ~Model

// Drawing

void XRender::DrawTextQueued(Vec3f pos, const XDrawTextInfo& ti, const char* format, va_list args)
{
    core::StackString512 temp;
    temp.appendFmt(format, args);

    X_ASSERT_NOT_NULL(pTextDrawList_);
    pTextDrawList_->addEntry(pos, ti, temp.c_str());
}

void XRender::DrawTextQueued(Vec3f pos, const XDrawTextInfo& ti, const char* text)
{
    X_ASSERT_NOT_NULL(pTextDrawList_);
    pTextDrawList_->addEntry(pos, ti, text);
}

void XRender::DrawAllocStats(Vec3f pos, const XDrawTextInfo& ti,
    const core::MemoryAllocatorStatistics& allocStats, const char* title)
{
    core::StackString512 str;
    core::HumanSize::Str temp;

    str.appendFmt("Num:%i" PRIuS "\n", allocStats.allocationCount_);
    str.appendFmt("Num(Max):%" PRIuS "\n", allocStats.allocationCountMax_);
    str.appendFmt("Physical:%s\n", core::HumanSize::toString(temp, allocStats.physicalMemoryAllocated_));
    str.appendFmt("Physical(Used):%s\n", core::HumanSize::toString(temp, allocStats.physicalMemoryUsed_));
    str.appendFmt("Virtual(Res):%s\n", core::HumanSize::toString(temp, allocStats.virtualMemoryReserved_));
    str.appendFmt("WasteAlign:%s\n", core::HumanSize::toString(temp, allocStats.wasteAlignment_));
    str.appendFmt("WasteUnused:%s\n", core::HumanSize::toString(temp, allocStats.wasteUnused_));
    str.appendFmt("Overhead:%s\n", core::HumanSize::toString(temp, allocStats.internalOverhead_));

    DrawTextQueued(pos + Vec3f(0, 15, 0), ti, str.c_str());

    XDrawTextInfo ti2 = ti;
    ti2.col = Col_Red;
    ti2.flags |= DrawTextFlags::FRAMED;

    DrawTextQueued(pos, ti2, title);
}

void XRender::FlushTextBuffer(void)
{
    X_ASSERT_NOT_NULL(pTextDrawList_);

    if (pTextDrawList_->isEmpty()) {
        return;
    }

    const XTextDrawList::TextEntry* entry;

    while ((entry = pTextDrawList_->getNextTextEntry()) != nullptr) {
        const Vec3f& pos = entry->pos;
        const char* pStr = entry->getText();

        float posX, posY;

        posX = pos.x;
        posY = pos.y;

        XDrawTextInfo ti;
        ti.flags = entry->flags;
        ti.col = entry->color;

        Draw2dText(posX, posY, pStr, ti);
    }

    pTextDrawList_->clear();
}

void XRender::Draw2dText(float posX, float posY, const char* pStr, const XDrawTextInfo& ti)
{
    static const float UIDRAW_TEXTSIZEFACTOR = 16.f;

    font::IFont* pFont = pDefaultFont_;
    if (!pFont)
        return;

    font::TextDrawContext ctx;
    ctx.SetColor(ti.col);
    ctx.SetCharWidthScale(1.0f);

    if (ti.flags.IsSet(DrawTextFlags::FRAMED)) {
        ctx.SetDrawFrame(true);
    }

    if (ti.flags.IsSet(DrawTextFlags::MONOSPACE)) {
        //	if (ti.flags.IsSet(DrawTextFlags::FIXED_SIZE))
        //		ctx.SetSizeIn800x600(false);
        ctx.SetSize(Vec2f(UIDRAW_TEXTSIZEFACTOR, UIDRAW_TEXTSIZEFACTOR));
        ctx.SetCharWidthScale(0.5f);
        ctx.SetProportional(false);

        //	if (ti.flags & eDrawText_800x600)
        //		ScaleCoordInternal(posX, posY);
    }
    else if (ti.flags.IsSet(DrawTextFlags::FIXED_SIZE)) {
        //		ctx.SetSizeIn800x600(false);
        ctx.SetSize(Vec2f(UIDRAW_TEXTSIZEFACTOR, UIDRAW_TEXTSIZEFACTOR));
        ctx.SetProportional(true);

        //	if (ti.flags & eDrawText_800x600)
        //		ScaleCoordInternal(posX, posY);
    }
    else {
        //		ctx.SetSizeIn800x600(true);
        ctx.SetProportional(false);
        ctx.SetCharWidthScale(0.5f);
        ctx.SetSize(Vec2f(UIDRAW_TEXTSIZEFACTOR, UIDRAW_TEXTSIZEFACTOR));
    }

    // align left/right/center
    bool align = (ti.flags & (DrawTextFlags::CENTER | DrawTextFlags::RIGHT | DrawTextFlags::CENTER_VER)).IsAnySet();

    if (align) {
        Vec2f textSize = pFont->GetTextSize(pStr, ctx);
        if (ti.flags.IsSet(DrawTextFlags::CENTER))
            posX -= textSize.x * 0.5f;
        else if (ti.flags.IsSet(DrawTextFlags::RIGHT))
            posX -= textSize.x;

        if (ti.flags.IsSet(DrawTextFlags::CENTER_VER))
            posY -= textSize.y * 0.5f;
    }

    pFont->DrawString(posX, posY, pStr, ctx);
}

// ~Drawing

// Shader Stuff

shader::XShaderItem XRender::LoadShaderItem(shader::XInputShaderResources& res)
{
    // we want a shader + texture collection plz!
    shader::XShaderItem item;

    item.pShader_ = ShaderMan_.s_pFixedFunction_;
    item.pShader_->addRef();
    item.technique_ = 2;
    item.pResources_ = ShaderMan_.createShaderResources(res);

    return item;
}

// ~Shader Stuff

X_NAMESPACE_END

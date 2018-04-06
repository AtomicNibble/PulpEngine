#pragma once

#ifndef X_SHADER_H_
#define X_SHADER_H_

#include <IShader.h>
#include <IDirectoryWatcher.h>

#include <String\StrRef.h>
#include <String\Lexer.h>
#include <String\StringHash.h>

#include <Containers\HashMap.h>
#include <Containers\Array.h>
#include <Util\Flags.h>

#include <../Common/Resources/BaseRenderAsset.h>
#include "XShaderBin.h"

#include <unordered_set>

X_NAMESPACE_DECLARE(core,
                    struct IConsoleCmdArgs;)

X_NAMESPACE_BEGIN(shader)

class XShaderManager;
struct ShaderSourceFile;
struct SourceFile;

#define SHADER_BIND_SAMPLER 0x4000

struct ConstbufType
{
    enum Enum
    {
        PER_FRAME,
        PER_BATCH,
        PER_INSTANCE,
        Num
    };
};

struct BlendInfo
{
    BlendType::Enum color;
    BlendType::Enum alpha;

    bool ParseBlendInfo(const char* name,
        const core::StackString512& key, const core::StackString512& value);
};

X_DECLARE_FLAGS(TechniquePrams)
(NAME, VERTEX_FNC, PIXEL_FNC);
X_DECLARE_FLAGS(ShaderStatus)
(NotCompiled, Compiling, AsyncCompileDone, UploadedToHW, ReadyToRock, FailedToCompile);

// in the shader name space so it's ok to call just: PrePro
X_DECLARE_ENUM(PreProType)
(Include, Define, Undef, If, IfDef, IfNDef, Else, EndIF);

class XShaderResources : public IRenderShaderResources
{
public:
    XShaderResources();

    // IRenderShaderResources
    virtual ~XShaderResources() X_OVERRIDE;

    virtual void release(void) X_OVERRIDE;

    virtual Color& getDiffuseColor() X_OVERRIDE
    {
        return diffuse;
    }
    virtual Color& getSpecularColor() X_OVERRIDE
    {
        return spec;
    }
    virtual Color& getEmissiveColor() X_OVERRIDE
    {
        return emissive;
    }

    virtual float& getSpecularShininess() X_OVERRIDE
    {
        return specShine;
    }
    virtual float& getGlow() X_OVERRIDE
    {
        return glow;
    }
    virtual float& getOpacity() X_OVERRIDE
    {
        return opacity;
    }

    virtual XTextureResource* getTexture(ShaderTextureIdx::Enum idx) const X_OVERRIDE
    {
        return pTextures[idx];
    }
    // ~IRenderShaderResources

    X_INLINE bool hasTexture(ShaderTextureIdx::Enum idx) const
    {
        return pTextures[idx] != nullptr;
    }

    // called from render thread.
    void RT_Release(void);

private:
    void freeAssets(void);

protected:
    X_NO_COPY(XShaderResources);
    X_NO_ASSIGN(XShaderResources);

    friend class XShaderManager;

    // the textures
    shader::XTextureResource* pTextures[ShaderTextureIdx::ENUM_COUNT]; // 8 x 3 = 24

    // 12 * 3 = 36
    Color diffuse;
    Color spec;
    Color emissive;

    // 12
    float specShine;
    float glow;
    float opacity;
};

class XShader;

class XHWShader : public core::XBaseAsset
{
public:
    XHWShader();

    static XHWShader* forName(const char* shader_name, const char* entry,
        const char* sourceFile, const Flags<TechFlag>& techFlag,
        ShaderType::Enum type, Flags<ILFlag> ILFlag, uint32_t sourceCrc);

    static const char* getProfileFromType(ShaderType::Enum type);

    X_INLINE const char* getName(void) const
    {
        return name_.c_str();
    }
    X_INLINE const char* getSourceFileName(void) const
    {
        return sourceFileName_.c_str();
    }
    X_INLINE const char* getEntryPoint(void) const
    {
        return entryPoint_.c_str();
    }

    X_INLINE ShaderStatus::Enum getStatus(void) const
    {
        return status_;
    }
    X_INLINE TechFlags getTechFlags(void) const
    {
        return techFlags_;
    }
    X_INLINE ShaderType::Enum getType(void) const
    {
        return type_;
    }
    X_INLINE InputLayoutFormat::Enum getILFormat(void) const
    {
        return IlFmt_;
    }

    X_INLINE uint32_t getNumRenderTargets(void) const
    {
        return numRenderTargets_;
    }
    X_INLINE uint32_t getNumSamplers(void) const
    {
        return numSamplers_;
    }
    X_INLINE uint32_t getNumConstantBuffers(void) const
    {
        return numConstBuffers_;
    }
    X_INLINE uint32_t getNumInputParams(void) const
    {
        return numInputParams_;
    }

    X_INLINE bool Compile(void)
    {
        return Compile(this);
    }

private:
    static bool Compile(XHWShader* pShader);

protected:
    static render::XRenderResourceContainer* s_pHWshaders;

    core::string name_;
    core::string sourceFileName_;
    core::string entryPoint_;
    uint32_t sourceCrc32_; // the crc of the source this was compiled from.

    // status
    ShaderStatus::Enum status_;
    // color, textured, skinned, instanced
    TechFlags techFlags_;
    // Vert / Pixel / Hull / Geo
    ShaderType::Enum type_;
    // POS_UV_COL_NOR
    InputLayoutFormat::Enum IlFmt_;

    // save info from shader reflection.
    uint32_t numRenderTargets_;
    uint32_t numSamplers_;
    uint32_t numConstBuffers_;
    uint32_t numInputParams_;
};

struct InputLayoutEntry
{
    const char* name;
    ILFlag::Enum flag;
};

struct TechFlagEntry
{
    const char* name;
    TechFlag::Enum flag;
};

struct PreProEntry
{
    const char* name;
    PreProType::Enum type;
};

struct PrePro
{
    PreProType::Enum type;
    core::string expression;
};

// a hlsl
struct SourceFile
{
    friend class XShaderManager;

    SourceFile() :
        sourceCrc32(0),
        includedFiles(g_rendererArena),
        prePros(g_rendererArena)
    {
    }

protected:
    core::string name;
    core::string fileName;
    core::string fileData;
    core::Array<SourceFile*> includedFiles;
    core::Array<PrePro> prePros;
    std::unordered_set<core::string, core::hash<core::string>> refrences;
    Flags<ILFlag> ILFlags;
    uint32_t sourceCrc32;
};

struct ShaderSourceFile
{
    friend class XShaderManager;

    ShaderSourceFile() :
        pFile_(nullptr),
        pHlslFile_(nullptr),
        sourceCrc32_(0),
        hlslSourceCrc32_(0),
        techniques_(g_rendererArena)
    {
    }

    struct Technique
    {
        Technique();

        core::string name_;
        core::string vertex_func_;
        core::string pixel_func_;

        BlendInfo src_;
        BlendInfo dst_;

        render::CullMode::Enum cullMode_;
        bool depth_write_;

        render::StateFlag state_;
        Flags<TechniquePrams> flags_;
        Flags<TechFlag> techFlags_;

        bool parse(core::XLexer& lex);
        bool processName(void);
    };

    X_INLINE size_t numTechs(void) const
    {
        return techniques_.size();
    }

protected:
    core::string name_;
    SourceFile* pFile_;
    SourceFile* pHlslFile_;
    uint32_t sourceCrc32_;
    uint32_t hlslSourceCrc32_;
    core::Array<Technique> techniques_;
};

struct XShaderTechniqueHW
{
    XShaderTechniqueHW()
    {
        core::zero_this(this);
    }

    void release(void)
    {
        core::SafeRelease(pVertexShader);
        core::SafeRelease(pPixelShader);
        core::SafeRelease(pGeoShader);
    }

    bool canDraw(void) const
    {
        bool canDraw = true;

        if (pVertexShader) {
            canDraw &= pVertexShader->getStatus() == ShaderStatus::ReadyToRock;
        }
        if (pPixelShader) {
            canDraw &= pPixelShader->getStatus() == ShaderStatus::ReadyToRock;
        }
        if (pGeoShader) {
            canDraw &= pGeoShader->getStatus() == ShaderStatus::ReadyToRock;
        }

        return canDraw;
    }

    void tryCompile(void)
    {
        if (pVertexShader) {
            if (pVertexShader->getStatus() != ShaderStatus::FailedToCompile) {
                if (pVertexShader->Compile()) {
                    IlFmt = pVertexShader->getILFormat();
                }
            }
        }
        if (pPixelShader) {
            if (pPixelShader->getStatus() != ShaderStatus::FailedToCompile) {
                pPixelShader->Compile();
            }
        }
    }

public:
    Flags<TechFlag> techFlags;
    Flags<ILFlag> ILFlags;
    InputLayoutFormat::Enum IlFmt;

    XHWShader* pVertexShader;
    XHWShader* pPixelShader;
    XHWShader* pGeoShader;
};

struct XShaderTechnique
{
    static const size_t TECH_MAX_SUB = 6;

    typedef core::FixedArray<XShaderTechniqueHW, TECH_MAX_SUB> TechHWList;

    XShaderTechnique() :
        pCurHwTech(nullptr)
    {
    }

    XShaderTechnique& operator=(const ShaderSourceFile::Technique& srcTech);

    void release(void)
    {
        TechHWList::iterator it;
        for (it = hwTechs.begin(); it != hwTechs.end(); ++it) {
            it->release();
        }
    }

    void append(const XShaderTechniqueHW& hwTech)
    {
        hwTechs.append(hwTech);
        pCurHwTech = hwTechs.end() - 1;
    }

    void resetCurHWTech(void)
    {
        pCurHwTech = hwTechs.begin();
    }

public:
    core::string name;
    core::StrHash nameHash;
    render::StateFlag state;
    render::CullMode::Enum cullMode;

    BlendInfo src;
    BlendInfo dst;

    XShaderTechniqueHW* pCurHwTech;
    TechHWList hwTechs;

    Flags<TechFlag> techFlags;
};

class XShader : public IShader
    , public core::XBaseAsset
{
    friend class XShaderManager;

public:
    XShader();
    ~XShader();

    virtual ShaderID getID() X_OVERRIDE
    {
        return XBaseAsset::getID();
    }
    virtual const int addRef() X_OVERRIDE
    {
        return XBaseAsset::addRef();
    }
    virtual const int release() X_OVERRIDE;

    virtual const char* getName() const X_OVERRIDE
    {
        return name_.c_str();
    }
    virtual VertexFormat::Enum getVertexFmt() X_OVERRIDE
    {
        return vertexFmt_;
    }

    // D3D Effects interface
    bool FXSetTechnique(const char* name, const TechFlags flag = TechFlags());
    bool FXSetTechnique(const core::StrHash& name, const TechFlags flag = TechFlags());
    bool FXBegin(uint32* uiPassCount, uint32 nFlags);
    bool FXBeginPass(uint32 uiPass);
    bool FXCommit(const uint32 nFlags);
    bool FXEndPass();
    bool FXEnd();

    bool FXSetVSFloat(const core::StrHash& NameParam, const Vec4f* pVecs, uint32_t numVecs);

private:
    X_INLINE size_t numTechs(void) const
    {
        return techs_.size();
    }

private:
    core::string name_;
    uint32_t sourceCrc32_;
    uint32_t hlslSourceCrc32_;

    SourceFile* pHlslFile_;

    VertexFormat::Enum vertexFmt_;

    core::Array<XShaderTechnique> techs_;
};

class XShaderManager : public core::IXHotReload
{
public:
    XShaderManager();
    ~XShaderManager();

    bool Init(void);
    bool Shutdown(void);

    XShader* forName(const char* name);
    SourceFile* loadRawSourceFile(const char* name, bool reload = false);

    bool sourceToString(core::string& str, const char* name);

    // IXHotReload
    void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
    // ~IXHotReload

    XShaderResources* createShaderResources(const XInputShaderResources& input);

private:
    bool loadCoreShaders(void);
    bool freeCoreShaders(void);
    bool freeSourcebin(void);
    void listShaders(void);
    void listShaderSources(void);

    //	bool loadShaderFile(const char* name);
    ShaderSourceFile* loadShaderFile(const char* name, bool reload = false);

    XShader* createShader(const char* name);
    XShader* loadShader(const char* name);
    XShader* reloadShader(const char* name);
    // ParseIncludesAndPrePro_r
    void ParseIncludesAndPrePro_r(SourceFile* file, core::Array<SourceFile*>& includedFiles,
        bool reload = false);

private:
    static void writeSourceToFile(core::XFile* f, const SourceFile* source);

    friend void Cmd_ListShaders(core::IConsoleCmdArgs* pArgs);
    friend void Cmd_ListShaderSources(core::IConsoleCmdArgs* pArgs);

private:
    typedef core::HashMap<core::string, SourceFile*> ShaderSourceMap;

    ShaderSourceMap Sourcebin_;
    render::XRenderResourceContainer shaders_;

public:
    core::Crc32* pCrc32_;

    static XShader* s_pDefaultShader_;
    static XShader* s_pDebugShader_;
    static XShader* s_pFixedFunction_;
    static XShader* s_pFont_;
    static XShader* s_pGui_;
    static XShader* s_pDefferedShader_;
    static XShader* s_pDefferedShaderVis_;

    static XShader* s_pWordShader_;
    static XShader* s_pModelShader_;
};

X_NAMESPACE_END

#endif // !X_SHADER_H_
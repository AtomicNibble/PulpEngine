#pragma once

#ifndef X_SHADER_I_H_
#define X_SHADER_I_H_

#include <Util\EnumMacros.h>
#include <String\StrRef.h>
#include <Math\XColor.h>

#include <Memory\NewAndDelete.h>

X_NAMESPACE_DECLARE(texture, class XTexture);

#include <ITexture.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{
    struct IShaderLib : public IConverter
    {
    };

    static const uint32_t MAX_SHADERS = 256;
    static const uint32_t MAX_HW_SHADERS = 512;
    static const uint32_t MAX_SHADER_SOURCE = 256;
    static const uint32_t MAX_SHADER_PERMS = 512;
    static const uint32_t MAX_SHADER_CB_PER_PERM = 16; // max const buffers allowed for a permatation.

    static const char* SOURCE_FILE_EXTENSION = "hlsl";
    static const char* SOURCE_INCLUDE_FILE_EXTENSION = "inc";
    static const char* SOURCE_MERGED_FILE_EXTENSION = "fxcb.hlsl"; // merged source.
    static const char* COMPILED_SHADER_FILE_EXTENSION = "fxcb";

    // we have shader params that will need to be updated at diffrent rates.
    // so we should group them.
    //
    // Some example shader parmas
    //
    //  genral matrix:
    //		ModelMat
    //		VeiwMat
    //		ProjectionMat
    //
    //  used for effects:
    //		Time
    //		Verte offset (for decals)

    //  Deffered rendering:
    //		width, height of viewport (used for calculating pos from depth)

    /*
// GL = global
// PB = Per-Batch
// PI = Per-Instance
// SI = Per-Instance Static
// PF = Per-Frame
// PM = Per-Material
// SK = Skin data
// MP = Morph data
*/

    X_DECLARE_FLAGS8(UpdateFreq)
    (
        // these need to be ordered in update freq high to low.
        FRAME,
        BATCH, // what is a batch?
        MATERIAL,
        INSTANCE,
        SKINDATA, // dunno what update rate of this is yet.
        UNKNOWN   // unknow is assumed worse case.
    );

    typedef Flags8<UpdateFreq> UpdateFreqFlags;

    // param can have multiple flags set eg: (Float & matrix)
    X_DECLARE_FLAGS8(ParamFlag)
    (
        FLOAT,
        INT,
        BOOL,
        MATRIX,
        VEC2,
        VEC3,
        VEC4
        // SAMPLER,
    );

    typedef Flags8<ParamFlag> ParamFlags;

    X_DECLARE_FLAGS(ParamType)
    ( // these are used as flags for dirty checks.

        Unknown,

        PF_worldToScreenMatrix, // worldspace to screenspace (view * proj)
        PF_screenToWorldMatrix, // screenspace to worldspace Inv(view * proj)
        PF_worldToCameraMatrix, // world to cameraspace (view)
        PF_cameraToWorldMatrix, // cameraspace back to worldspace (inView)
        PF_viewMatrix,
        PF_projectionMatrix,

        PF_Time,        // float ms (total time passed)
        PF_FrameTime,   // float ms (frame delta)
        PF_FrameTimeUI, // float ms (frame detla with UI scale);
        PF_ScreenSize,  // x,y, 0.5/x, 0.5 /y
        PF_CameraPos,

        PI_objectToWorldMatrix, // ?
        PI_worldMatrix,
        PI_worldViewProjectionMatrix

    );

    typedef Flags<ParamType> ParamTypeFlags;

    // I support diffrent vertex formats
    // for use by the engine, not so much assets.

    // P = Position
    // C = Color
    // T = TexCord
    // N = Normal
    // TB = tanget & bi-normal
    //
    // Layout:
    //         Type | Num | Format
    // Eg: P3F -> Position 3 floats.

    X_DECLARE_ENUM8(VertexFormat)
    (
        P3F_T3F, // used in Aux Geo.

        // 16bit tex coords
        P3F_T2S,
        P3F_T2S_C4B,
        P3F_T2S_C4B_N3F,
        P3F_T2S_C4B_N3F_TB3F,

        // same as above but using compressed normals.
        P3F_T2S_C4B_N10,
        P3F_T2S_C4B_N10_TB10,

        // 32bit texcoords
        P3F_T2F_C4B, // used by gui.

        // double coords
        P3F_T4F_C4B_N3F,

        NONE);

    // -----------------------------------------------------------------------

#if 0
    X_DECLARE_ENUM8(ShaderTextureIdx)
    (
        DIFFUSE,
        BUMP
        );
#endif

    X_DECLARE_ENUM8(ShaderType)
    (
        Vertex,
        Pixel,
        Geometry,
        Hull,
        Domain,
        UnKnown);

    X_DECLARE_FLAGS8(ShaderStage)
    (
        Vertex,
        Pixel,
        Geometry,
        Hull,
        Domain);

    typedef Flags8<ShaderStage> ShaderStageFlags;

    // not sure where to put this.
    // it's used in material lib.
    X_INLINE constexpr ShaderStage::Enum staderTypeToStageFlag(ShaderType::Enum type)
    {
        return static_cast<ShaderStage::Enum>(1 << type);
    }

    // make sure helper works correct :)
    static_assert(staderTypeToStageFlag(ShaderType::Vertex) == ShaderStage::Vertex, "Enum to flag helper logic error");
    static_assert(staderTypeToStageFlag(ShaderType::Pixel) == ShaderStage::Pixel, "Enum to flag helper logic error");
    static_assert(staderTypeToStageFlag(ShaderType::Geometry) == ShaderStage::Geometry, "Enum to flag helper logic error");
    static_assert(staderTypeToStageFlag(ShaderType::Hull) == ShaderStage::Hull, "Enum to flag helper logic error");
    static_assert(staderTypeToStageFlag(ShaderType::Domain) == ShaderStage::Domain, "Enum to flag helper logic error");

    static const char* DEFAULT_SHADER_ENTRY[ShaderStage::FLAGS_COUNT] = {
        "vs_main",
        "ps_main",
        "gs_main",
        "hs_main",
        "ds_main",
    };

    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //
    // These ILFlags are automatically detected whne parsing the hlsl source file.
    // It looks for macros that start with IL_*(Normal, BiNormal, Color)
    //
    // When it finds these flags it knows the source supports been compiled with AND without that flags.
    // Which lets you define a shader that both works with and without Normal stream.
    //
    // The source will be compiled first with no flags defined which will be the minimum the shader requires as input.
    // Then the shader works out what the required streams are for that compiled source.
    //
    //  It then repeats this step each time adding a flag follwed by calculating the IL for that compiled source.
    //
    //	Example:
    //
    //		if the hlsl contains:
    //			#if IL_NORMAL
    //			#if IL_BINORMAL
    //
    //
    //		compile0:
    //				flags: <none>
    //				detectedIL:	POS_UV_COL
    //
    //		compile1:
    //				flags: IL_NORMAL
    //				detectedIL:	POS_UV_COL_NORM
    //
    //		compile2:
    //				flags: IL_NORMAL, IL_BINORMAL
    //				detectedIL:	POS_UV_COL_NORM_TAN_BI
    //
    //
    //	the order the flags are define between each compile pass matches the order of the enum.
    //	So you won't get: IL_BINORMAL defined before IL_NORMAL is defined, they accumulate in order.
    //
    //	But it's totally possible to just have only IL_BINORMAL defined, if the shader requires normals but not binormals.
    //

    X_DECLARE_FLAGS(ILFlag)
    (
        Uv2,
        Normal,
        BiNormal,
        Color);

    typedef Flags<ILFlag> ILFlags;

    X_DECLARE_FLAG_OPERATORS(ILFlags);

    X_DECLARE_ENUM8(InputLayoutFormat)
    (
        Invalid,

        POS_UV,
        POS_UV_COL,
        POS_UV_COL_NORM,
        POS_UV_COL_NORM_TAN,
        POS_UV_COL_NORM_TAN_BI,

        POS_UV2_COL_NORM,

        NONE // shaders that don't take verts.
    );

    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //
    //	These flags are to allow a shader to be compiled supporting difffrent features.
    //
    //	This means a shader can be select that supports skinning only
    //	Or the same shader that supports skinning and instancing.
    //	The flags are defined in the .shader file with the tech name currently.
    //
    //	Every permatation of the shader will be compiled:
    //
    //	"Fill(Textured)" -> compiled with: <none>, X_TEXTURED
    //	"Fill(Color)" -> compiled with: <none>, X_COLOR
    //	"Fill(Color, Textured)" -> compiled with: <none>, X_COLOR, X_TEXTURED, X_TEXTURED|X_COLOR
    //
    //
    //

#if 0
X_DECLARE_FLAGS(TechFlag)(
	Color, 
	Textured, 
	Skinned, 
	Instanced
);

typedef Flags<TechFlag> TechFlags;
#endif

    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

    typedef int32_t ShaderID;
    typedef uintptr_t TechHandle;

    class CBufferLink;
    class Buffer;
    class Sampler;
    class Texture;

    struct IShaderPermatation
    {
        template<typename T>
        using LineraArray = core::Array<T, core::ArrayAllocator<T>, core::growStrat::FixedLinear<4>>;

        typedef LineraArray<CBufferLink> CBufLinksArr;
        typedef LineraArray<Buffer> BufferArr;
        typedef LineraArray<Sampler> SamplerArr;
        typedef LineraArray<Texture> TexutreArr;

        virtual ~IShaderPermatation() = default;

        virtual InputLayoutFormat::Enum getILFmt(void) const X_ABSTRACT;
        virtual const CBufLinksArr& getCbufferLinks(void) const X_ABSTRACT;
        virtual size_t getNumBuffersTotal(void) const X_ABSTRACT;
        virtual const BufferArr& getBuffers(ShaderType::Enum type) const X_ABSTRACT;
        virtual const SamplerArr& getSamplers(void) const X_ABSTRACT;
        virtual const TexutreArr& getTextures(void) const X_ABSTRACT;
    };

    struct IShaderTech
    {
        virtual ~IShaderTech() = default;

        virtual IShaderPermatation* getPermatation(VertexFormat::Enum vertexFmt) X_ABSTRACT;
    };

    struct IShader
    {
        virtual ~IShader() = default;

        virtual const core::string& getName(void) const X_ABSTRACT;

        // tech stuff.
        virtual IShaderTech* getTech(const char* pName) X_ABSTRACT;

        //	virtual VertexFormat::Enum getVertexFmt() X_ABSTRACT;
        //	virtual ShaderType::Enum getType() X_ABSTRACT;
    };

    // ------------------ new..  ------------------------------------------

    X_DECLARE_FLAGS8(Permatation)
    (
        VertStreams,
        HwSkin,
        Instanced,
        Textured);

    typedef Flags8<Permatation> PermatationFlags;

    X_DECLARE_FLAG_OPERATORS(PermatationFlags);

    X_DECLARE_FLAGS8(CompileFlag)
    (
        Debug,
        TreatWarningsAsErrors,
        OptimizationLvl0,
        OptimizationLvl1,
        OptimizationLvl2,
        OptimizationLvl3,
        PackMatrixRowMajor,
        PackMatrixColumnMajor);

    typedef Flags8<CompileFlag> CompileFlags;

    X_DECLARE_FLAG_OPERATORS(CompileFlags);

    inline constexpr CompileFlags COMPILE_DEBUG_FLAGS = CompileFlag::Debug | CompileFlag::OptimizationLvl0;
    inline constexpr CompileFlags COMPILE_RELEASE_FLAGS = CompileFlag::TreatWarningsAsErrors | CompileFlag::OptimizationLvl2;
    inline constexpr CompileFlags COMPILE_BAKE_FLAGS = CompileFlag::TreatWarningsAsErrors | CompileFlag::OptimizationLvl3;


    struct IHWShader
    {
        virtual ~IHWShader() = default;
    };

    struct IShaderSource
    {
        virtual ~IShaderSource() = default;

        virtual ILFlags getILFlags(void) const X_ABSTRACT;
    };

    typedef std::array<IShaderSource*, ShaderStage::FLAGS_COUNT> ShaderSourceArr;
    typedef std::array<IHWShader*, ShaderStage::FLAGS_COUNT> ShaderStagesArr;

} // namespace shader

X_NAMESPACE_END

#endif // X_SHADER_I_H_

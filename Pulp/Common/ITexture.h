#pragma once

#ifndef _X_TEXTURE_I_H_
#define _X_TEXTURE_I_H_

#include <Util\Flags.h>
#include <Util\FlagsMacros.h>
#include <Math\XVector.h>
#include <Util\GenericUtil.h>
#include <String\Path.h>

#include <IConverterModule.h>

X_NAMESPACE_DECLARE(core,
    struct XFile)

X_NAMESPACE_BEGIN(texture)

// defaults.
static const char* TEX_DEFAULT_DIFFUSE = "default/$default";
static const char* TEX_DEFAULT_BUMP = "default/$identitynormal";
static const char* TEX_DEFAULT_WHITE = "default/$white";

static const uint32_t TEX_MIN_DIMENSIONS = 4;
static const uint32_t TEX_MAX_DIMENSIONS = 4096 * 2;
static const uint32_t TEX_MAX_MIPS = 14;
static const uint32_t TEX_MAX_FACES = 6; // cubes (vol's not supported yet)
static const uint32_t TEX_MAX_DEPTH = 1;
static const uint32_t TEX_MAX_LOADED_IMAGES = 1 << 13; // max images that can be loaded.

struct IImgLib : public IConverter
{
};

X_DECLARE_ENUM8(TextureType)
(
    TCube,
    T1D,
    T2D,
    T3D,

    UNKNOWN);

// these are UNORM by default.
// except bc6 which is uf16, so yer every things unsigned by default.
// !! this is not safe to edit without all CI image rebuild. !!
// !! adding stuff at end is safe !!
// Few places need updating after changes here:
// * ImgLib::Converter.cpp
// * ImgLib::TextureUtil.cpp
// * RenderDll::TextureManager.cpp
X_DECLARE_ENUM8(Texturefmt)
(
    A8,

    R8G8, // _UNORM,
    R8G8_TYPELESS,
    R8G8_SNORM,
    R8G8_UNIT,
    R8G8_SINT,

    R16G16_FLOAT,
    R16G16,      //_UNORM,
    R16G16_SRGB, // _UNORM
    R16G16_SNORM,
    R16G16_SINT,
    R16G16_UINT,
    R16G16_TYPELESS,

    // for HDR / BC6 input
    R16G16B16A16_FLOAT,

    R8G8B8,

    B8G8R8,

    R8G8B8A8,      // _UNORM,
    R8G8B8A8_SRGB, // _UNORM
    R8G8B8A8_SNORM,
    R8G8B8A8_TYPELESS,
    R8G8B8A8_SINT,
    R8G8B8A8_UINT,

    A8R8G8B8,

    B8G8R8A8,      // _UNORM,
    B8G8R8A8_SRGB, // _UNORM
    B8G8R8A8_TYPELESS,

    ATI2,    // DXN_YX / 3dc
    ATI2_XY, // DXN_XY

    BC1,      // _UNORM,		// DXT1
    BC1_SRGB, // _UNORM
    BC1_TYPELESS,

    BC2,      // _UNORM,		// DXT3
    BC2_SRGB, // _UNORM
    BC2_TYPELESS,

    BC3,      // _UNORM,		// DXT5
    BC3_SRGB, // _UNORM
    BC3_TYPELESS,

    BC4, // _UNORM,
    BC4_SNORM,
    BC4_TYPELESS,

    BC5, // _UNORM,
    BC5_SNORM,
    BC5_TYPELESS,

    // Dx11
    BC6, // _UF16,
    BC6_SF16,
    BC6_TYPELESS,

    BC7, // _UNORM,
    BC7_SRGB,
    BC7_TYPELESS,

    R10G10B10A2, // _UNORM
    R10G10B10A2_UINT,
    R10G10B10A2_TYPELESS,

    R24G8_TYPELESS,
    D24_UNORM_S8_UNIT,
    D32_FLOAT,

    UNKNOWN);

X_DECLARE_FLAGS(TexFlag)
(
    NOMIPS,
    FORCE_MIPS,
    DONT_RESIZE,

    ALPHA,
    NORMAL,

    STREAMING,        // streaming in progress
    STREAMABLE,       // can be streamed
    HI_MIP_STREAMING, // only high mips can be streamed.
    FORCE_STREAM,     // force stream even if only one mip

    // can be used to force a img that would normaly be stream to be sync loaded.
    // useful for enforcing sync loads for required assets.
    DONT_STREAM,

    CI_IMG,

    // ------- runtime only ---------

    RENDER_TARGET,

    LOAD_FAILED);

typedef Flags<TexFlag> TextureFlags;

X_DECLARE_FLAG_OPERATORS(TextureFlags);

X_DECLARE_ENUM(ImgFileFormat)
(
    CI,
    DDS,
    PNG,
    JPG,
    PSD,
    TGA,

    UNKNOWN);

/*
Streaming:

some images can be streamed in, not all since it don't make sense to stream.
for example UI images we want the whole image before a load finishes.

but otherwise we will load the smallest collection of mipmaps syncrosly, 
then stream in larger mips on demand.

we support 4 blocks of data so a image of 4096x4096 will be:

block 4: 4096x4096
block 3: 2048x2048
block 2: 1024x1204
block 1: 512x512 256x256 128x128 64x64 32x32 16x16 8x8 4x4 2x2 1x1

when we sotorre the diffrent blocks we should order them how we preditate them to 
be loaded. for example if a collection of models are near each other.
it makes sense to store the bigger res blocks for the images the models use
next to each other in the file, giving us a good chance of reduced seeking 
when streaming images in for that 'area' of the map.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Here is a thought:

do we need to store the small mips when close to the object?

meanes we could place other image data in it's place, allowing us to make
better use of vram.

What are the downsides?

1. if we are storing another image where the small mips use to be, and we are wanting
	to start using the smaller mips, we are going to have to put the old image back

	a potential solution could be to force the gpu to use the higer images
	untill we have got the correct small mips back into memory.

we could just do it but not be aggressive about it.
so say we have block 4 been used currently we would only allow block 1
to be overwriteen, then as soon as block 3 is been used start getting 
block 1 in vram again.


bacon for thought...

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

*/

typedef int TexID;
class XTextureFile;

struct ITextureFmt
{
    typedef core::Array<uint8_t> DataVec;

    virtual ~ITextureFmt() = default;

    virtual const char* getExtension(void) const X_ABSTRACT;
    virtual ImgFileFormat::Enum getSrcFmt(void) const X_ABSTRACT;
    virtual bool canLoadFile(const core::Path<char>& path) const X_ABSTRACT;
    virtual bool canLoadFile(const DataVec& fileData) const X_ABSTRACT;
    virtual bool loadTexture(core::XFile* file, XTextureFile& imgFile, core::MemoryArenaBase* swapArena) X_ABSTRACT;

    virtual bool canWrite(void) const
    {
        return false;
    }
    virtual bool saveTexture(core::XFile* file, const XTextureFile& imgFile, core::MemoryArenaBase* swapArena)
    {
        X_UNUSED(file, imgFile, swapArena);
        X_ASSERT_UNREACHABLE();
        return false;
    };
};

X_NAMESPACE_END

#endif // !_X_TEXTURE_I_H_

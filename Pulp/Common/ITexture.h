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
struct XFile;
)

X_NAMESPACE_BEGIN(texture)

// defaults.
static const char* TEX_DEFAULT_DIFFUSE = "default/$default";
static const char* TEX_DEFAULT_BUMP = "default/$identitynormal";
static const char* TEX_DEFAULT_WHITE = "default/$white";

static const uint32_t	 TEX_MIN_DIMENSIONS = 4;
static const uint32_t	 TEX_MAX_DIMENSIONS = 4096;
static const uint32_t	 TEX_MAX_MIPS = 13;
static const uint32_t	 TEX_MAX_FACES = 6; // cubes (vol's not supported yet)
static const uint32_t	 TEX_MAX_DEPTH = 1; 
static const uint32_t	 TEX_MAX_LOADED_IMAGES = 1 << 13; // max images that can be loaded.



struct IImgLib : public IConverter
{

};



X_DECLARE_ENUM8(TextureType) (
	TCube,
	T1D,
	T2D,
	T3D,

	UNKNOWN
);

// these are UNORM by default.
// except bc6 which is uf16, so yer every things unsigned by default.
// !! this is not safe to edit without all CI image rebuild. !!
// !! adding stuff at end is safe !!
// Few places need updating after changes here:
// * ImgLib::Converter.cpp
// * ImgLib::TextureUtil.cpp
// * RenderDll::TextureManager.cpp
X_DECLARE_ENUM8(Texturefmt) (
	A8,

	R8G8, // _UNORM,
	R8G8_TYPELESS,
	R8G8_SNORM,
	R8G8_UNIT,
	R8G8_SINT,

	R16G16_FLOAT,
	R16G16, //_UNORM,
	R16G16_SRGB, // _UNORM
	R16G16_SNORM,
	R16G16_SINT,
	R16G16_UINT,
	R16G16_TYPELESS,

	// for HDR / BC6 input
	R16G16B16A16_FLOAT,

	R8G8B8,

	B8G8R8,

	R8G8B8A8, // _UNORM,
	R8G8B8A8_SRGB, // _UNORM
	R8G8B8A8_SNORM,
	R8G8B8A8_TYPELESS,
	R8G8B8A8_SINT,
	R8G8B8A8_UINT,

	A8R8G8B8,

	B8G8R8A8, // _UNORM,
	B8G8R8A8_SRGB, // _UNORM
	B8G8R8A8_TYPELESS,

	ATI2,		// DXN_YX / 3dc
	ATI2_XY,	// DXN_XY

	BC1, // _UNORM,		// DXT1
	BC1_SRGB, // _UNORM
	BC1_TYPELESS,

	BC2, // _UNORM,		// DXT3
	BC2_SRGB, // _UNORM
	BC2_TYPELESS,

	BC3, // _UNORM,		// DXT5
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

	UNKNOWN
);

X_DECLARE_FLAGS(TexFlag)(
	NOMIPS, 
	FORCE_MIPS,
	DONT_RESIZE,
	
	ALPHA,
	NORMAL, 

	STREAMING,			// streaming in progress
	STREAMABLE,			// can be streamed
	HI_MIP_STREAMING,	// only high mips can be streamed.
	FORCE_STREAM,		// force stream even if only one mip

	// can be used to force a img that would normaly be stream to be sync loaded.
	// useful for enforcing sync loads for required assets.
	DONT_STREAM,

	CI_IMG,

	// ------- runtime only ---------

	RENDER_TARGET, 

	LOAD_FAILED
);

typedef Flags<TexFlag> TextureFlags;

X_DECLARE_FLAG_OPERATORS(TextureFlags);


X_DECLARE_ENUM(ImgFileFormat) (
	CI,
	DDS,
	PNG,
	JPG,
	PSD,
	TGA,

	UNKNOWN
);

/*
We can request a texture and one is always returned.
it may point to the deafault texture.

then we can request it to be loaded.
this will attempt to find the file and load it into memory.

but rendering is still safe, since a texture object will always point to
a texture be it default or the real one.

we support loading of many diffrent formats, in dev mode.
to make working on things quicker.

how to find out what format img X is currently be saved as.

Possible strats:

1.  define a order of loading for images:
	EG: .ci .dds .tga .psd

	then load which ever one we find first.

	this has the problem of if they have made a .ci version that will be loaded.
	instead of the one they are updating.
	
	maybe do reverse order:

	.psd .tga .dds .ci

	that means anytime there is a psd that will be loaded, if not .ci is loaded.
	and when they want to release we switch to .ci loading only.

2.  since a image is only used from a material.
	we could maybe store the format they first added the image as.

	then just check if we can find that format.

	this has the problem of if they switch format they will need to update the 
	material.

Summary:

	if we use order loading of images, people might get confused if they change format
	and old format is getting loaded, meaning they will have to delete it.

	which would mean they would have to be aware of the order formats are looked for.
	so I think it's best to make it so they have to update the material, at the cost of 
	a extra step we help remove a 'scratches head' momments for dev's.

	This step also only has to be done when they want to change the format
	the image is AFTER they have previously created the material, so the issues is
	small.

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
	very quickly otherwise we could have what i'm gonna call 'cherry-pop' where a
	diffrent texture is beening used untill old data is placed back.

	a potential solution could be to force the gpu to use the higer images
	untill we have got the correct small mips back into memory.

we could just do it but not be aggressive about it.
so say we have block 4 been used currently we would only allow block 1
to be overwriteen, then as soon as block 3 is been used start getting 
block 1 in vram again.


bacon for thought...

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

we refrrence textures by name, but can we also do a id system?
to work along side the names for better speed, but still able to print names 
for convience.

we could store it's hash to save computing it.

IdSystem:

if we do id's we should do 32bit ones giving us 4mill possible asset id's.
might not be enougth HUHUHUH. 

but then what is going to provide these id's?

I guess we can load a texture by name then once loaded it's assigned a ID.
which means we load by name but can then just use the id of the texture.

we could just keep a pointer to the interface but we save 4 bytes on 64bit
and virtual calls, by using a 32bit id.

so the answer is the resource manager will give us the ID.

which we then can use to render, WHOOP.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


we have difrrent texuture objects.

* FileTexuture: XTextureFile
		this is a texture we have loaded from a file.

		and of hte loaders must return a object of this type.
		we then use this to make a Device Texture.

* Device texture: -> XTexture
		This texture is on the GPU device.
		it can be binded and used for rendering.


The code for handling the textures is common code,
only the code for diffrent API will be diffrent.

so we could split the deffinition between 2 files one for common stuff,
another has the API shizz, gonna have to change my folder structure a bit.


*/

typedef int TexID;
class XTextureFile;

struct ITextureFmt
{
	typedef core::Array<uint8_t> DataVec;

	virtual ~ITextureFmt() {}

	virtual const char* getExtension(void) const X_ABSTRACT;
	virtual ImgFileFormat::Enum getSrcFmt(void) const X_ABSTRACT;
	virtual bool canLoadFile(const core::Path<char>& path) const X_ABSTRACT;
	virtual bool canLoadFile(const DataVec& fileData) const X_ABSTRACT;
	virtual bool loadTexture(core::XFile* file, XTextureFile& imgFile, core::MemoryArenaBase* swapArena) X_ABSTRACT;

	virtual bool canWrite(void) const { return false; }
	virtual bool saveTexture(core::XFile* file, const XTextureFile& imgFile, core::MemoryArenaBase* swapArena) {
		X_UNUSED(file);
		X_UNUSED(imgFile);
		X_UNUSED(swapArena);
		X_ASSERT_UNREACHABLE();
		return false;
	};

};



X_NAMESPACE_END

#endif // !_X_TEXTURE_I_H_

#pragma once

#ifndef _X_TEXTURE_I_H_
#define _X_TEXTURE_I_H_

#include <Util\Flags.h>
#include <Util\FlagsMacros.h>
#include <Math\XVector.h>
#include <Util\GenericUtil.h>
#include <String\Path.h>

X_NAMESPACE_DECLARE(core,
struct XFile;
)

X_NAMESPACE_BEGIN(texture)

// defaults.
static const char* TEX_DEFAULT_DIFFUSE = "Textures/default.dds";
static const char* TEX_DEFAULT_BUMP = "Textures/$identitynormal.dds";

static const uint32_t	 TEX_MAX_DIMENSIONS = 4096;
static const uint32_t	 TEX_MAX_MIPS = 13;
static const uint32_t	 TEX_MAX_FACES = 6; // cubes (vol's not supported yet)
static const uint32_t	 TEX_MAX_DEPTH = 1; 

struct TextureType
{
	enum Enum : uint8_t
	{
		TCube = 1,
		T1D,
		T2D,
		T3D,

		UNKNOWN
	};
};

X_DECLARE_ENUM8(Texturefmt) (
R8G8B8,
R8G8B8A8,
A8,
A8R8G8B8,

B8G8R8,
B8G8R8A8,

ATI2,		// DXN_YX / 3dc
ATI2_XY,	// DXN_XY

BC1,		// DXT1
BC2,		// DXT3
BC3,		// DXT5
BC4,
BC4_SNORM,
BC5,
BC5_SNORM,
// Dx11
BC6,
BC6_UF16,
BC6_SF16,

BC7,
BC7_UNORM,
BC7_UNORM_SRGB,

R16G16F,
R10G10B10A2,

UNKNOWN,
);

X_DECLARE_FLAGS(TexFlag)(NOMIPS, LOAD_FAILED, DONT_RESIZE,
	FORCE_MIPS, ALPHA, NORMAL, DONT_STREAM, TEX_FONT, 
	FILTER_POINT, FILTER_LINEAR, FILTER_BILINEAR, FILTER_TRILINEAR, 
	RENDER_TARGET, 
	CI_IMG
);

typedef Flags<TexFlag> TextureFlags;


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

struct ITexture
{
	virtual ~ITexture(){};

	// textures are shared, we ref count them so we know when we are done.
	virtual const int addRef() X_ABSTRACT;
	virtual const int release() X_ABSTRACT;
	virtual const int forceRelease() X_ABSTRACT;


	// the resource id.
	virtual const TexID getTexID() const X_ABSTRACT;

	virtual const char* getName() const X_ABSTRACT;
	virtual const Vec2<uint16_t> getDimensions() const X_ABSTRACT;
	virtual const int getWidth() const X_ABSTRACT;
	virtual const int getHeight() const X_ABSTRACT;
	virtual const int getNumFaces() const X_ABSTRACT;
	virtual const int getNumMips() const X_ABSTRACT;
	virtual const int getDepth() const X_ABSTRACT;
	virtual const int getDataSize() const X_ABSTRACT;
	virtual const bool isLoaded() const X_ABSTRACT;
	virtual const bool IsShared() const X_ABSTRACT;
	virtual const bool IsStreamable() const X_ABSTRACT;

	virtual const TextureType::Enum getTextureType() const X_ABSTRACT;
	virtual const TextureFlags getFlags() const X_ABSTRACT;
	virtual const Texturefmt::Enum getFormat() const X_ABSTRACT;

};


struct XTextureFile;

// Interface for a texture loader.
// allows me to add / remove diffrent texture loaders with ease.
struct ITextureLoader
{
	virtual ~ITextureLoader() {}

	virtual bool canLoadFile(const core::Path<char>& path) const X_ABSTRACT;
	virtual XTextureFile* loadTexture(core::XFile* file) X_ABSTRACT;
};


struct ITextureFmt
{
	virtual ~ITextureFmt() {}

	virtual bool canLoadFile(const core::Path<char>& path) const X_ABSTRACT;
	virtual bool loadTexture(core::XFile* file, XTextureFile& imgFile, core::MemoryArenaBase* swapArena) X_ABSTRACT;

	virtual bool canWrite(void) const { return false; }
	virtual bool saveTexture(core::XFile* file, const XTextureFile& imgFile) {
		X_UNUSED(file);
		X_UNUSED(imgFile);
		X_ASSERT_UNREACHABLE();
		return false;
	};
};



X_NAMESPACE_END

#endif // !_X_TEXTURE_I_H_

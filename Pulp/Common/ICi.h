#pragma once

#ifndef X_CI_IMG_H_
#define X_CI_IMG_H_

X_NAMESPACE_BEGIN(texture)

static const char* CI_FILE_EXTENSION = "ci";
static const uint32_t CI_FOURCC = X_TAG('c', 'i', 'm', 'g');
static const uint8_t CI_VERSION = 3;
// these are format limits.
// global limtis must still be respected when loading.
static const uint32_t CI_MAX_DIMENSIONS = UINT16_MAX;
static const uint32_t CI_MAX_MIPS = UINT8_MAX;
static const uint32_t CI_MAX_FACES = UINT8_MAX;

struct CITexureHeader
{
    CITexureHeader()
    {
        core::zero_this(this);
    }

    uint32 fourCC;
    uint8 version;
    Texturefmt::Enum format;
    uint8 Mips;
    uint8 Faces;

    TextureFlags Flags;

    X_DISABLE_WARNING(4201)
    union
    {
        struct
        {
            uint16_t width;
            uint16_t height;
        };
        struct
        {
            Vec2<uint16_t> size;
        };
    };
    X_ENABLE_WARNING(4201)

    uint32 DataSize;    // the size of all the data.
    uint32 FaceSize;    // face size
    uint32 crc32;       // data + flags.
    uint32 __Unused[2]; // room for expansion.

    bool isValid(void) const
    {
        return fourCC == CI_FOURCC;
    }
};

X_ENSURE_SIZE(CITexureHeader, 36);

X_NAMESPACE_END

#endif // !X_CI_IMG_H_

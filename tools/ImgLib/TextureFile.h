#pragma once

#ifndef X_TEXTURE_FILE_H_
#define X_TEXTURE_FILE_H_

#include <ITexture.h>
#include <Containers\Array.h>

#include "Util\TextureUtil.h"

X_NAMESPACE_BEGIN(texture)

class XTextureFile
{
    typedef core::Array<uint8_t, core::ArrayAlignedAllocatorFixed<uint8_t, 16>> DataArrAligned;

public:
    X_INLINE XTextureFile(core::MemoryArenaBase* arena);
    X_INLINE ~XTextureFile();

public:
    // allocatos memory to hold (N mips) * N faces
    X_INLINE void resize(void);
    X_INLINE void allocMipBuffers(void); // resizes the buffer to have space for all mip lvl's, keeping data from top mip.
    X_INLINE void dropTopMip(void);
    X_INLINE void dropMips(bool shrinkMem = false);
    X_INLINE bool flipVertical(core::MemoryArenaBase* swap);
    X_INLINE void clear(void);
    X_INLINE void free(void);
    X_INLINE const bool isValid(void) const;

    X_INLINE Vec2<uint16_t> getSize(void) const;
    X_INLINE Vec2<uint16_t> getMipDim(size_t mipIdx) const;
    X_INLINE int32_t getWidth(void) const;
    X_INLINE int32_t getHeight(void) const;
    X_INLINE uint8_t getNumFaces(void) const;
    X_INLINE uint8_t getDepth(void) const;
    X_INLINE uint8_t getNumMips(void) const;
    X_INLINE size_t getDataSize(void) const;
    X_INLINE TextureFlags getFlags(void) const;
    X_INLINE Texturefmt::Enum getFormat(void) const;
    X_INLINE TextureType::Enum getType(void) const;

    X_INLINE const uint8_t* getFace(size_t face) const;
    X_INLINE uint8_t* getFace(size_t face);

    X_INLINE const uint8_t* getLevel(size_t face, size_t mip) const;
    X_INLINE uint8_t* getLevel(size_t face, size_t mip);

    X_INLINE size_t getFaceSize(void) const;
    X_INLINE size_t getLevelSize(size_t mip) const;
    X_INLINE size_t getLevelRowbytes(size_t mip) const;

    X_INLINE void setSize(const Vec2<uint16_t> size);
    X_INLINE void setWidth(const uint16_t width);
    X_INLINE void setHeigth(const uint16_t height);
    X_INLINE void setFlags(TextureFlags flags);
    X_INLINE void setType(TextureType::Enum type);
    X_INLINE void setFormat(Texturefmt::Enum format);

    X_INLINE void setNumFaces(const int32_t num);
    X_INLINE void setDepth(const int32_t depth);
    X_INLINE void setNumMips(const int32_t num);

    X_INLINE void swap(XTextureFile& oth);

    X_NO_COPY(XTextureFile);
    X_NO_ASSIGN(XTextureFile);

private:
    X_INLINE void updateOffsets(void);

private:
    uint32_t mipOffsets_[TEX_MAX_MIPS];
    uint32_t faceOffsets_[TEX_MAX_FACES];

    // 4
    Vec2<uint16_t> size_; // 4
    TextureFlags flags_;
    TextureType::Enum type_;
    Texturefmt::Enum format_;
    uint8_t numMips_;
    uint8_t depth_;    // Volume x,y,w
    uint8_t numFaces_; // Cube maps aka 6 faces.
    bool sizeValid_;
    uint8_t _PAD[2];

    DataArrAligned data_;
    // pad to 128 bytes.
    //	uint8_t				__PAD[12];
};

static const size_t size_check = sizeof(XTextureFile);

// X_ENSURE_SIZE(XTextureFile, 128);

X_NAMESPACE_END

#include "TextureFile.inl"

#endif // !X_TEXTURE_FILE_H_
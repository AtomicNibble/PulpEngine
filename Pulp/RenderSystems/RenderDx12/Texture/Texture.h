#pragma once

#include <ITexture.h>

#include "GpuResource.h"


X_NAMESPACE_DECLARE(render,
	class DescriptorAllocator;
);

X_NAMESPACE_BEGIN(texture)

class TextureManager;

	class X_ALIGNED_SYMBOL(Texture, 64) : public ::texture::ITexture
	{
		friend TextureManager;

	public:
		Texture(const char* pName, TextureFlags flags);
		Texture(core::string name, TextureFlags flags);
		~Texture();

		void destroy(void);

		// temp maybe
		X_INLINE const TexID getTexID(void) const X_FINAL { return id_; };
		X_INLINE const bool IsShared(void) const X_FINAL { return false; }


		X_INLINE const core::string& getName(void) const X_FINAL;
		X_INLINE const Vec2<uint16_t> getDimensions(void) const X_FINAL;
		X_INLINE const int32_t getWidth(void) const X_FINAL;
		X_INLINE const int32_t getHeight(void) const X_FINAL;
		X_INLINE const int32_t getNumFaces(void) const X_FINAL;
		X_INLINE const int32_t getDepth(void) const X_FINAL;
		X_INLINE const int32_t getNumMips(void) const X_FINAL;
		X_INLINE const int32_t getDataSize(void) const X_FINAL;

		X_INLINE const bool isLoaded(void) const X_FINAL;
		X_INLINE const bool IsStreamable(void) const X_FINAL;

		X_INLINE const TextureType::Enum getTextureType(void) const X_FINAL;
		X_INLINE const TextureFlags getFlags(void) const X_FINAL;
		X_INLINE const Texturefmt::Enum getFormat(void) const X_FINAL;
		const DXGI_FORMAT getFormatDX(void) const;

		X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& getSRV(void) const;

		X_INLINE const int32_t getID(void) const;
		X_INLINE void setID(TexID id);

		X_INLINE render::GpuResource& getGpuResource(void);
	protected:
		X_INLINE void setSRV(D3D12_CPU_DESCRIPTOR_HANDLE& srv);

		X_INLINE void setFormat(Texturefmt::Enum fmt);
		X_INLINE void setType(TextureType::Enum type);
		X_INLINE void setWidth(uint16_t width);
		X_INLINE void setHeight(uint16_t height);
		X_INLINE void setDepth(uint8_t depth);
		X_INLINE void setNumFaces(uint8_t faces);
		X_INLINE void setNumMips(uint8_t mips);


	private:
		render::GpuResource	resource_;
		D3D12_CPU_DESCRIPTOR_HANDLE hCpuDescriptorHandle_;

		core::string		fileName_;
		TexID				id_;
		Vec2<uint16_t>		dimensions_;
		uint32_t			datasize_; // size of the higest mip in bytes.
		TextureType::Enum	type_;
		TextureFlags		flags_;
		Texturefmt::Enum 	format_;
		uint8_t				numMips_;
		uint8_t				depth_;
		uint8_t				numFaces_;
	};



X_NAMESPACE_END

#include "Texture.inl"
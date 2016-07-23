#pragma once

X_NAMESPACE_DECLARE(core,
	struct ICVar;
)


X_NAMESPACE_BEGIN(texture)


	class TextureVars
	{
	public:
		TextureVars();
		~TextureVars() = default;

		void RegisterVars(void);

		X_INLINE bool allowRawImgLoading(void) const;

	private:
		int32_t allowRawImgLoading_;

	};


X_NAMESPACE_END


#include "TextureVars.inl"
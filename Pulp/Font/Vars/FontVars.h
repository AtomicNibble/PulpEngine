#pragma once


X_NAMESPACE_DECLARE(core,
	struct ICVar;
)

X_NAMESPACE_BEGIN(font)


class FontVars
{
public:
	FontVars();
	~FontVars() = default;

	void registerVars(void);

	X_INLINE int32_t glyphCacheSize(void) const;
	X_INLINE bool glyphCachePreWarm(void) const;

private:
	int32_t glyphCacheSize_;
	int32_t glyphCachePreWarm_;
	
};


X_NAMESPACE_END

#include "FontVars.inl"
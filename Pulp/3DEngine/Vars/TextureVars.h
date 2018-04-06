#pragma once

X_NAMESPACE_DECLARE(core,
                    struct ICVar;)

X_NAMESPACE_BEGIN(engine)

class TextureVars
{
public:
    TextureVars();
    ~TextureVars() = default;

    void registerVars(void);

    X_INLINE bool allowRawImgLoading(void) const;
    X_INLINE bool allowFmtDDS(void) const;
    X_INLINE bool allowFmtPNG(void) const;
    X_INLINE bool allowFmtJPG(void) const;
    X_INLINE bool allowFmtPSD(void) const;
    X_INLINE bool allowFmtTGA(void) const;

private:
    int32_t allowRawImgLoading_;

    // disable / enable foramts.
    int32_t allowFmtDDS_;
    int32_t allowFmtPNG_;
    int32_t allowFmtJPG_;
    int32_t allowFmtPSD_;
    int32_t allowFmtTGA_;
};

X_NAMESPACE_END

#include "TextureVars.inl"
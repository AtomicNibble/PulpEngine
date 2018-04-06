

X_NAMESPACE_BEGIN(texture)

namespace Converter
{
    X_INLINE float Filter::width(void) const
    {
        return width_;
    }

    // -------------------------------------------------------------------

    X_INLINE float Kernel1::valueAt(uint32_t x) const
    {
        return data_[x];
    }

    X_INLINE int32_t Kernel1::windowSize(void) const
    {
        return windowSize_;
    }

    X_INLINE float Kernel1::width(void) const
    {
        return width_;
    }

    // -------------------------------------------------------------------

    X_INLINE float Kernel2::valueAt(uint x, uint y) const
    {
        return data_[y * windowSize_ + x];
    }

    X_INLINE uint32_t Kernel2::windowSize(void) const
    {
        return windowSize_;
    }

    // -------------------------------------------------------------------

    X_INLINE float PolyphaseKernel::valueAt(uint32_t column, uint32_t x) const
    {
        return data_[column * windowSize_ + x];
    }

    X_INLINE int32_t PolyphaseKernel::windowSize(void) const
    {
        return windowSize_;
    }

    X_INLINE float PolyphaseKernel::width(void) const
    {
        return width_;
    }

    X_INLINE uint32_t PolyphaseKernel::length(void) const
    {
        return length_;
    }

} // namespace Converter

X_NAMESPACE_END
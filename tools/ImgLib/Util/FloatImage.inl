

X_NAMESPACE_BEGIN(texture)

namespace Converter
{
    X_INLINE uint32_t FloatImage::width(void) const
    {
        return width_;
    }

    X_INLINE uint32_t FloatImage::height(void) const
    {
        return height_;
    }

    X_INLINE uint32_t FloatImage::depth(void) const
    {
        return depth_;
    }

    X_INLINE uint32_t FloatImage::componentCount(void) const
    {
        return componentCount_;
    }

    X_INLINE uint32_t FloatImage::pixelCount(void) const
    {
        return pixelCount_;
    }

    X_INLINE const float* FloatImage::channel(uint32_t component) const
    {
        X_ASSERT(component < componentCount_, "Component index out of range")(component, componentCount_); 
        return data_.ptr() + component * pixelCount_;
    }

    X_INLINE float* FloatImage::channel(uint32_t component)
    {
        X_ASSERT(component < componentCount_, "Component index out of range")(component, componentCount_); 
        return data_.ptr() + component * pixelCount_;
    }

    X_INLINE const float* FloatImage::plane(uint32_t component, uint32_t z) const
    {
        X_ASSERT(z < depth_, "Depth index out of range")(z, depth_); 
        return channel(component) + z * width_ * height_;
    }

    X_INLINE float* FloatImage::plane(uint32_t component, uint32_t z)
    {
        X_ASSERT(z < depth_, "Depth index out of range")(z, depth_); 
        return channel(component) + z * width_ * height_;
    }

    X_INLINE const float* FloatImage::scanline(uint32_t component, uint32_t y, uint32_t z) const
    {
        X_ASSERT(y < height_, "Height index out of range")(y, height_); 

        return plane(component, z) + y * width_;
    }

    X_INLINE float* FloatImage::scanline(uint32_t component, uint32_t y, uint32_t z)
    {
        X_ASSERT(y < height_, "Height index out of range")(y, height_); 
        return plane(component, z) + y * width_;
    }

    X_INLINE uint32_t FloatImage::index(uint32_t x, uint32_t y, uint32_t z) const
    {
        X_ASSERT(x < width_, "Width index out of range")(x, width_); 
        X_ASSERT(y < height_, "Height index out of range")(y, height_); 
        X_ASSERT(z < depth_, "Depth index out of range")(z, depth_); 
        const uint32_t idx = (z * height_ + y) * width_ + x;
        X_ASSERT(idx < pixelCount_, "Pixel index out of range")(idx, pixelCount_); 
        return idx;
    }

} // namespace Converter

X_NAMESPACE_END
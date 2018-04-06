#pragma once

#ifndef _X_COLOR_PACKING_UTIL_H_
#define _X_COLOR_PACKING_UTIL_H_

template<typename T>
struct CHANTRAIT
{
};

template<>
struct CHANTRAIT<uint8_t>
{
    typedef uint32_t Sum;
    typedef uint32_t Accum;
    typedef int32_t SignedSum;
    static constexpr uint8_t max()
    {
        return 255;
    }
    static constexpr uint8_t convert(uint8_t v)
    {
        return v;
    }
    static constexpr uint8_t convert(uint16_t v)
    {
        return static_cast<uint8_t>(v / 257);
    }
    static constexpr uint8_t convert(float v)
    {
        return static_cast<uint8_t>(v * 255);
    }
    static constexpr uint8_t grayscale(uint8_t r, uint8_t g, uint8_t b)
    {
        return static_cast<uint8_t>((r * 54 + g * 183 + b * 19) >> 8);
    } // luma coefficients from Rec. 709
    static constexpr uint8_t premultiply(uint8_t c, uint8_t a)
    {
        return a * c / 255u;
    }
    static constexpr uint8_t inverse(uint8_t c)
    {
        return static_cast<uint8_t>(~c);
    }
};

template<>
struct CHANTRAIT<uint16_t>
{
    typedef uint32_t Sum;
    typedef uint32_t Accum;
    typedef int32_t SignedSum;
    static constexpr uint16_t max()
    {
        return 65535;
    }
    static constexpr uint16_t convert(uint8_t v)
    {
        return static_cast<uint16_t>((v << 8) | v);
    }
    static constexpr uint16_t convert(uint16_t v)
    {
        return v;
    }
    static constexpr uint16_t convert(float v)
    {
        return static_cast<uint16_t>(v * 65535);
    }
    static constexpr uint16_t grayscale(uint16_t r, uint16_t g, uint16_t b)
    {
        return static_cast<uint16_t>((r * 6966 + g * 23436 + b * 2366) >> 15);
    } // luma coefficients from Rec. 709
};

template<>
struct CHANTRAIT<float>
{
    typedef float Sum;
    typedef float Accum;
    typedef float SignedSum;
    static constexpr float max()
    {
        return 1.0f;
    }
    static constexpr float convert(uint8_t v)
    {
        return v / 255.0f;
    }
    static constexpr float convert(uint16_t v)
    {
        return v / 65535.0f;
    }
    static constexpr float convert(float v)
    {
        return v;
    }
    static constexpr float grayscale(float r, float g, float b)
    {
        return r * 0.2126f + g * 0.7152f + b * 0.0722f;
    } // luma coefficients from Rec. 709
    //! Calculates the multiplied version of a color component \a c by alpha \a a
    static constexpr float premultiply(float c, float a)
    {
        return c * a;
    }
    static constexpr float inverse(float c)
    {
        return 1.0f - c;
    }
};

#endif // !_X_COLOR_PACKING_UTIL_H_

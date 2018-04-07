#pragma once

#ifndef _X_MATH_COLOR_H_
#define _X_MATH_COLOR_H_

#include "XColorPacking.h"

template<typename T>
struct ColorT
{
    T r, g, b, a;

    X_INLINE ColorT();
    X_INLINE ColorT(T r, T g, T b, T a);
    //	template<typename FromT>
    //	X_INLINE ColorT(FromT r, FromT g, FromT b, FromT a);

    // for specilisation.
    X_INLINE ColorT(const ColorT<T>& src);

    template<typename FromT>
    X_INLINE ColorT(const ColorT<FromT>& src);
    // Can use it like rgb
    X_INLINE ColorT(T r, T g, T b);

    template<typename FromT>
    X_INLINE explicit ColorT(const Vec3<FromT>& src, FromT alpha);
    template<typename FromT>
    X_INLINE explicit ColorT(const Vec4<FromT>& src);

    X_INLINE void set(T _r, T _g, T _b, T _a);
    X_INLINE void set(const ColorT<T>& rhs);

    X_INLINE operator T*();
    X_INLINE operator const T*() const;

    X_INLINE ColorT<T> operator=(const ColorT<T>& rhs);

    X_INLINE const T& operator[](int i) const;
    X_INLINE T& operator[](int i);

    X_INLINE ColorT<T> operator+(const ColorT<T>& rhs) const;
    X_INLINE ColorT<T> operator-(const ColorT<T>& rhs) const;
    X_INLINE ColorT<T> operator*(const ColorT<T>& rhs) const;
    X_INLINE ColorT<T> operator/(const ColorT<T>& rhs) const;
    X_INLINE const ColorT<T>& operator+=(const ColorT<T>& rhs);
    X_INLINE const ColorT<T>& operator-=(const ColorT<T>& rhs);
    X_INLINE const ColorT<T>& operator*=(const ColorT<T>& rhs);
    X_INLINE const ColorT<T>& operator/=(const ColorT<T>& rhs);
    X_INLINE ColorT<T> operator+(T rhs) const;
    X_INLINE ColorT<T> operator-(T rhs) const;
    X_INLINE ColorT<T> operator*(T rhs) const;

    template<typename OtherT>
    X_INLINE ColorT<T> operator*(OtherT rhs) const;

    X_INLINE ColorT<T> operator/(T rhs) const;
    X_INLINE const ColorT<T>& operator+=(T rhs);
    X_INLINE const ColorT<T>& operator-=(T rhs);
    X_INLINE const ColorT<T>& operator*=(T rhs);
    X_INLINE const ColorT<T>& operator/=(T rhs);

    X_INLINE bool operator==(const ColorT<T>& rhs) const;
    X_INLINE bool operator!=(const ColorT<T>& rhs) const;

    X_INLINE float length(void) const;
    // tests for zero-length
    X_INLINE ColorT<T>& normalize(void);
    X_INLINE ColorT<T> premultiplied(void) const;

    X_INLINE typename CHANTRAIT<T>::Accum lengthSquared() const;
    X_INLINE ColorT<T> lerp(float fact, const ColorT<T>& d) const;

    X_INLINE bool compare(const ColorT<T>& oth, const T epsilon = math<T>::EPSILON);

    // expose packing util.
    X_INLINE uint8_t asRGB332(void) const;
    X_INLINE uint16_t asARGB4444(void) const;
    X_INLINE uint16_t asRGB555(void) const;
    X_INLINE uint16_t asRGB565(void) const;
    X_INLINE uint32_t asBGR888(void) const;
    X_INLINE uint32_t asRGB888(void) const;
    X_INLINE uint32_t asABGR8888(void) const;
    X_INLINE uint32_t asARGB8888(void) const;

    // darken with negative values etc.
    X_INLINE void shade(const float percent);

    // ><><><><><<><><><><><><><><<><><><><><><><><<><><><><><><
    static ColorT<T> zero(void);
    static ColorT<T> black(void);
    static ColorT<T> white(void);
    static ColorT<T> gray(T value, T alpha = CHANTRAIT<T>::max());

    //! Returns a ColorA from a hexadecimal-encoded RGB triple. For example, red is 0xFF0000
    static ColorT<T> hex(uint32_t hexValue);

    //! Returns a ColorA from a hexadecimal-encoded ARGB ordering. For example, 50% transparent red is 0x80FF0000
    static ColorT<T> hexA(uint32_t hexValue);

    static bool fromString(const char* pBegin, const char* pEnd, ColorT<T>& out, bool slient = true);
};


#include "XColor.inl" // needs to be included before the operators below.

typedef ColorT<float32_t> Color;
typedef ColorT<float32_t> Colorf;
typedef ColorT<uint8_t> Color8u;
typedef ColorT<uint16_t> Color16u;

// Operators
template<typename T, typename Y>
inline ColorT<T> operator*(Y s, const ColorT<T>& c)
{
    return ColorT<T>(s * c.r, s * c.g, s * c.b, s * c.a);
}

template<>
inline Color8u operator*(float s, const Color8u& c)
{
    return Color8u(
        static_cast<uint8_t>((float)c.r * s),
        static_cast<uint8_t>((float)c.g * s),
        static_cast<uint8_t>((float)c.b * s),
        static_cast<uint8_t>((float)c.a * s));
}

// define some colors?
// got them from: http://prideout.net/archive/colors.php
#define Col_Aliceblue Colorf(0.941f, 0.973f, 1.000f)
#define Col_Antiquewhite Colorf(0.980f, 0.922f, 0.843f)
#define Col_Aqua Colorf(0.000f, 1.000f, 1.000f)
#define Col_Aquamarine Colorf(0.498f, 1.000f, 0.831f)
#define Col_Azure Colorf(0.941f, 1.000f, 1.000f)
#define Col_Beige Colorf(0.961f, 0.961f, 0.863f)
#define Col_Bisque Colorf(1.000f, 0.894f, 0.769f)
#define Col_Black Colorf(0.000f, 0.000f, 0.000f)
#define Col_Blanchedalmond Colorf(1.000f, 0.922f, 0.804f)
#define Col_Blue Colorf(0.000f, 0.000f, 1.000f)
#define Col_Blueviolet Colorf(0.541f, 0.169f, 0.886f)
#define Col_Brown Colorf(0.647f, 0.165f, 0.165f)
#define Col_Burlywood Colorf(0.871f, 0.722f, 0.529f)
#define Col_Cadetblue Colorf(0.373f, 0.620f, 0.627f)
#define Col_Chartreuse Colorf(0.498f, 1.000f, 0.000f)
#define Col_Chocolate Colorf(0.824f, 0.412f, 0.118f)
#define Col_Coral Colorf(1.000f, 0.498f, 0.314f)
#define Col_Cornflowerblue Colorf(0.392f, 0.584f, 0.929f)
#define Col_Cornsilk Colorf(1.000f, 0.973f, 0.863f)
#define Col_Crimson Colorf(0.863f, 0.078f, 0.235f)
#define Col_Cyan Colorf(0.000f, 1.000f, 1.000f)
#define Col_Darkblue Colorf(0.000f, 0.000f, 0.545f)
#define Col_Darkcyan Colorf(0.000f, 0.545f, 0.545f)
#define Col_Darkgoldenrod Colorf(0.722f, 0.525f, 0.043f)
#define Col_Darkgray Colorf(0.663f, 0.663f, 0.663f)
#define Col_Darkgreen Colorf(0.000f, 0.392f, 0.000f)
#define Col_Darkgrey Colorf(0.663f, 0.663f, 0.663f)
#define Col_Darkkhaki Colorf(0.741f, 0.718f, 0.420f)
#define Col_Darkmagenta Colorf(0.545f, 0.000f, 0.545f)
#define Col_Darkolivegreen Colorf(0.333f, 0.420f, 0.184f)
#define Col_Darkorange Colorf(1.000f, 0.549f, 0.000f)
#define Col_Darkorchid Colorf(0.600f, 0.196f, 0.800f)
#define Col_Darkred Colorf(0.545f, 0.000f, 0.000f)
#define Col_Darksalmon Colorf(0.914f, 0.588f, 0.478f)
#define Col_Darkseagreen Colorf(0.561f, 0.737f, 0.561f)
#define Col_Darkslateblue Colorf(0.282f, 0.239f, 0.545f)
#define Col_Darkslategray Colorf(0.184f, 0.310f, 0.310f)
#define Col_Darkslategrey Colorf(0.184f, 0.310f, 0.310f)
#define Col_Darkturquoise Colorf(0.000f, 0.808f, 0.820f)
#define Col_Darkviolet Colorf(0.580f, 0.000f, 0.827f)
#define Col_Deeppink Colorf(1.000f, 0.078f, 0.576f)
#define Col_Deepskyblue Colorf(0.000f, 0.749f, 1.000f)
#define Col_Dimgray Colorf(0.412f, 0.412f, 0.412f)
#define Col_Dimgrey Colorf(0.412f, 0.412f, 0.412f)
#define Col_Dodgerblue Colorf(0.118f, 0.565f, 1.000f)
#define Col_Firebrick Colorf(0.698f, 0.133f, 0.133f)
#define Col_Floralwhite Colorf(1.000f, 0.980f, 0.941f)
#define Col_Forestgreen Colorf(0.133f, 0.545f, 0.133f)
#define Col_Fuchsia Colorf(1.000f, 0.000f, 1.000f)
#define Col_Gainsboro Colorf(0.863f, 0.863f, 0.863f)
#define Col_Ghostwhite Colorf(0.973f, 0.973f, 1.000f)
#define Col_Gold Colorf(1.000f, 0.843f, 0.000f)
#define Col_Goldenrod Colorf(0.855f, 0.647f, 0.125f)
#define Col_Gray Colorf(0.502f, 0.502f, 0.502f)
#define Col_Green Colorf(0.000f, 0.502f, 0.000f)
#define Col_Greenyellow Colorf(0.678f, 1.000f, 0.184f)
#define Col_Grey Colorf(0.502f, 0.502f, 0.502f)
#define Col_Honeydew Colorf(0.941f, 1.000f, 0.941f)
#define Col_Hotpink Colorf(1.000f, 0.412f, 0.706f)
#define Col_Indianred Colorf(0.804f, 0.361f, 0.361f)
#define Col_Indigo Colorf(0.294f, 0.000f, 0.510f)
#define Col_Ivory Colorf(1.000f, 1.000f, 0.941f)
#define Col_Khaki Colorf(0.941f, 0.902f, 0.549f)
#define Col_Lavender Colorf(0.902f, 0.902f, 0.980f)
#define Col_Lavenderblush Colorf(1.000f, 0.941f, 0.961f)
#define Col_Lawngreen Colorf(0.486f, 0.988f, 0.000f)
#define Col_Lemonchiffon Colorf(1.000f, 0.980f, 0.804f)
#define Col_Lightblue Colorf(0.678f, 0.847f, 0.902f)
#define Col_Lightcoral Colorf(0.941f, 0.502f, 0.502f)
#define Col_Lightcyan Colorf(0.878f, 1.000f, 1.000f)
#define Col_Lightgoldenrodyellow Colorf(0.980f, 0.980f, 0.824f)
#define Col_Lightgray Colorf(0.827f, 0.827f, 0.827f)
#define Col_Lightgreen Colorf(0.565f, 0.933f, 0.565f)
#define Col_Lightgrey Colorf(0.827f, 0.827f, 0.827f)
#define Col_Lightpink Colorf(1.000f, 0.714f, 0.757f)
#define Col_Lightsalmon Colorf(1.000f, 0.627f, 0.478f)
#define Col_Lightseagreen Colorf(0.125f, 0.698f, 0.667f)
#define Col_Lightskyblue Colorf(0.529f, 0.808f, 0.980f)
#define Col_Lightslategray Colorf(0.467f, 0.533f, 0.600f)
#define Col_Lightslategrey Colorf(0.467f, 0.533f, 0.600f)
#define Col_Lightsteelblue Colorf(0.690f, 0.769f, 0.871f)
#define Col_Lightyellow Colorf(1.000f, 1.000f, 0.878f)
#define Col_Lime Colorf(0.000f, 1.000f, 0.000f)
#define Col_Limegreen Colorf(0.196f, 0.804f, 0.196f)
#define Col_Linen Colorf(0.980f, 0.941f, 0.902f)
#define Col_Magenta Colorf(1.000f, 0.000f, 1.000f)
#define Col_Maroon Colorf(0.502f, 0.000f, 0.000f)
#define Col_Mediumaquamarine Colorf(0.400f, 0.804f, 0.667f)
#define Col_Mediumblue Colorf(0.000f, 0.000f, 0.804f)
#define Col_Mediumorchid Colorf(0.729f, 0.333f, 0.827f)
#define Col_Mediumpurple Colorf(0.576f, 0.439f, 0.859f)
#define Col_Mediumseagreen Colorf(0.235f, 0.702f, 0.443f)
#define Col_Mediumslateblue Colorf(0.482f, 0.408f, 0.933f)
#define Col_Mediumspringgreen Colorf(0.000f, 0.980f, 0.604f)
#define Col_Mediumturquoise Colorf(0.282f, 0.820f, 0.800f)
#define Col_Mediumvioletred Colorf(0.780f, 0.082f, 0.522f)
#define Col_Midnightblue Colorf(0.098f, 0.098f, 0.439f)
#define Col_Mintcream Colorf(0.961f, 1.000f, 0.980f)
#define Col_Mistyrose Colorf(1.000f, 0.894f, 0.882f)
#define Col_Moccasin Colorf(1.000f, 0.894f, 0.710f)
#define Col_Navajowhite Colorf(1.000f, 0.871f, 0.678f)
#define Col_Navy Colorf(0.000f, 0.000f, 0.502f)
#define Col_Oldlace Colorf(0.992f, 0.961f, 0.902f)
#define Col_Olive Colorf(0.502f, 0.502f, 0.000f)
#define Col_Olivedrab Colorf(0.420f, 0.557f, 0.137f)
#define Col_Orange Colorf(1.000f, 0.647f, 0.000f)
#define Col_Orangered Colorf(1.000f, 0.271f, 0.000f)
#define Col_Orchid Colorf(0.855f, 0.439f, 0.839f)
#define Col_Palegoldenrod Colorf(0.933f, 0.910f, 0.667f)
#define Col_Palegreen Colorf(0.596f, 0.984f, 0.596f)
#define Col_Paleturquoise Colorf(0.686f, 0.933f, 0.933f)
#define Col_Palevioletred Colorf(0.859f, 0.439f, 0.576f)
#define Col_Papayawhip Colorf(1.000f, 0.937f, 0.835f)
#define Col_Peachpuff Colorf(1.000f, 0.855f, 0.725f)
#define Col_Peru Colorf(0.804f, 0.522f, 0.247f)
#define Col_Pink Colorf(1.000f, 0.753f, 0.796f)
#define Col_Plum Colorf(0.867f, 0.627f, 0.867f)
#define Col_Powderblue Colorf(0.690f, 0.878f, 0.902f)
#define Col_Purple Colorf(0.502f, 0.000f, 0.502f)
#define Col_Red Colorf(1.000f, 0.000f, 0.000f)
#define Col_Rosybrown Colorf(0.737f, 0.561f, 0.561f)
#define Col_Royalblue Colorf(0.255f, 0.412f, 0.882f)
#define Col_Saddlebrown Colorf(0.545f, 0.271f, 0.075f)
#define Col_Salmon Colorf(0.980f, 0.502f, 0.447f)
#define Col_Sandybrown Colorf(0.957f, 0.643f, 0.376f)
#define Col_Seagreen Colorf(0.180f, 0.545f, 0.341f)
#define Col_Seashell Colorf(1.000f, 0.961f, 0.933f)
#define Col_Sienna Colorf(0.627f, 0.322f, 0.176f)
#define Col_Silver Colorf(0.753f, 0.753f, 0.753f)
#define Col_Skyblue Colorf(0.529f, 0.808f, 0.922f)
#define Col_Slateblue Colorf(0.416f, 0.353f, 0.804f)
#define Col_Slategray Colorf(0.439f, 0.502f, 0.565f)
#define Col_Slategrey Colorf(0.439f, 0.502f, 0.565f)
#define Col_Snow Colorf(1.000f, 0.980f, 0.980f)
#define Col_Springgreen Colorf(0.000f, 1.000f, 0.498f)
#define Col_Steelblue Colorf(0.275f, 0.510f, 0.706f)
#define Col_Tan Colorf(0.824f, 0.706f, 0.549f)
#define Col_Teal Colorf(0.000f, 0.502f, 0.502f)
#define Col_Thistle Colorf(0.847f, 0.749f, 0.847f)
#define Col_Tomato Colorf(1.000f, 0.388f, 0.278f)
#define Col_Turquoise Colorf(0.251f, 0.878f, 0.816f)
#define Col_Violet Colorf(0.933f, 0.510f, 0.933f)
#define Col_Wheat Colorf(0.961f, 0.871f, 0.702f)
#define Col_White Colorf(1.000f, 1.000f, 1.000f)
#define Col_Whitesmoke Colorf(0.961f, 0.961f, 0.961f)
#define Col_Yellow Colorf(1.000f, 1.000f, 0.000f)
#define Col_Yellowgreen Colorf(0.604f, 0.804f, 0.196f)


#endif // !_X_MATH_COLOR_H_

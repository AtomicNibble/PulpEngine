
template<typename T>
X_INLINE ColorT<T>::ColorT() :
	r(0), g(0), b(0), a(0)
{

}


template<typename T>
X_INLINE ColorT<T>::ColorT(T r, T g, T b, T a) :
	r(r), g(g), b(b), a(a)
{

}

template<>
X_INLINE ColorT<uint8_t>::ColorT(const ColorT<uint8_t>& src) :
	r(src.r), g(src.g), b(src.b), a(src.a)
{

}

template<>
X_INLINE ColorT<float32_t>::ColorT(const ColorT<float32_t>& src) :
	r(src.r), g(src.g), b(src.b), a(src.a)
{

}


template<>
template<>
X_INLINE ColorT<uint8_t>::ColorT(const ColorT<float32_t>& src)
{
	a = CHANTRAIT<uint8_t>::convert(src.a);
	r = CHANTRAIT<uint8_t>::convert(src.r);
	g = CHANTRAIT<uint8_t>::convert(src.g);
	b = CHANTRAIT<uint8_t>::convert(src.b);
}


template<>
template<>
X_INLINE ColorT<float32_t>::ColorT(const ColorT<uint8_t>& src)
{
	a = CHANTRAIT<float32_t>::convert(src.a);
	r = CHANTRAIT<float32_t>::convert(src.r);
	g = CHANTRAIT<float32_t>::convert(src.g);
	b = CHANTRAIT<float32_t>::convert(src.b);
}


// Can use it like rgb
template<typename T>
X_INLINE ColorT<T>::ColorT(T r, T g, T b) :
	r(r), g(g), b(b), a(CHANTRAIT<T>::convert(1.0f))
{
}

template<typename T>
template<typename FromT>
X_INLINE ColorT<T>::ColorT(const Vec3<FromT>& src, FromT alpha) :
	r(CHANTRAIT<T>::convert(src.x)),
	g(CHANTRAIT<T>::convert(src.y)),
	b(CHANTRAIT<T>::convert(src.z)),
	a(CHANTRAIT<T>::convert(alpha))
{
}

template<typename T>
template<typename FromT>
X_INLINE ColorT<T>::ColorT(const Vec4<FromT>& src) :
	r(CHANTRAIT<T>::convert(src.x)),
	g(CHANTRAIT<T>::convert(src.y)),
	b(CHANTRAIT<T>::convert(src.z)),
	a(CHANTRAIT<T>::convert(src.w))
{
}




template<typename T>
X_INLINE void ColorT<T>::set(T _r, T _g, T _b, T _a)
{
	r = _r; g = _g; b = _b; a = _a;
}

template<typename T>
X_INLINE void ColorT<T>::set(const ColorT<T>& rhs)
{
	r = rhs.r; g = rhs.g; b = rhs.b; a = rhs.a;
}
template<typename T>
X_INLINE ColorT<T>::operator T*()
{
	return (T*) this;
}

template<typename T>
X_INLINE ColorT<T>::operator const T*() const
{
	return (const T*) this;
}

template<typename T>
X_INLINE ColorT<T> ColorT<T>::operator=(const ColorT<T>& rhs)
{
	r = rhs.r;
	g = rhs.g;
	b = rhs.b;
	a = rhs.a;
	return *this;
}

template<typename T>
X_INLINE const T& ColorT<T>::operator[](int i) const {
	X_ASSERT(i >= 0 && i < 4, "out of range")(i);
	return (&r)[i];
}

template<typename T>
X_INLINE T& ColorT<T>::operator[](int i) {
	X_ASSERT(i >= 0 && i < 4, "out of range")(i);
	return (&r)[i];
}


template<typename T>
X_INLINE ColorT<T>	ColorT<T>::operator+(const ColorT<T> &rhs) const {
	return ColorT<T>(r + rhs.r, g + rhs.g, b + rhs.b, a + rhs.a);
}

template<typename T>
X_INLINE ColorT<T>	ColorT<T>::operator-(const ColorT<T> &rhs) const {
	return ColorT<T>(r - rhs.r, g - rhs.g, b - rhs.b, a - rhs.a);
}

template<typename T>
X_INLINE ColorT<T>	ColorT<T>::operator*(const ColorT<T> &rhs) const {
	return ColorT<T>(r * rhs.r, g * rhs.g, b * rhs.b, a * rhs.a);
}

template<>
X_INLINE ColorT<uint8> ColorT<uint8>::operator*(const ColorT<uint8>& rhs) const {
	int nr = (r * rhs.r) / 255;
	int ng = (g * rhs.g) / 255;
	int nb = (b * rhs.b) / 255;
	int na = (a * rhs.a) / 255;
	return ColorT<uint8>(
		safe_static_cast<uint8_t, int>(nr),
		safe_static_cast<uint8_t, int>(ng),
		safe_static_cast<uint8_t, int>(nb),
		safe_static_cast<uint8_t, int>(na)
	);
}


template<typename T>
X_INLINE ColorT<T>	ColorT<T>::operator/(const ColorT<T> &rhs) const {
	return ColorT<T>(r / rhs.r, g / rhs.g, b / rhs.b, a / rhs.a);
}

template<typename T>
X_INLINE const ColorT<T>&	ColorT<T>::operator+=(const ColorT<T> &rhs) {
	r += rhs.r; g += rhs.g; b += rhs.b; a += rhs.a;
	return *this;
}

template<typename T>
X_INLINE const ColorT<T>&	ColorT<T>::operator-=(const ColorT<T> &rhs) {
	r -= rhs.r; g -= rhs.g; b -= rhs.b; a -= rhs.a;
	return *this;
}

template<typename T>
X_INLINE const ColorT<T>&	ColorT<T>::operator*=(const ColorT<T> &rhs) {
	r *= rhs.r; g *= rhs.g; b *= rhs.b; a *= rhs.a;
	return *this;
}

template<typename T>
X_INLINE const ColorT<T>&	ColorT<T>::operator/=(const ColorT<T> &rhs)
{
	r /= rhs.r; g /= rhs.g; b /= rhs.b; a /= rhs.a;
	return *this;
}

template<typename T>
X_INLINE ColorT<T>	ColorT<T>::operator+(T rhs) const
{
	return ColorT<T>(r + rhs, g + rhs, b + rhs, a + rhs);
}

template<typename T>
X_INLINE ColorT<T>	ColorT<T>::operator-(T rhs) const
{
	return ColorT<T>(r - rhs, g - rhs, b - rhs, a - rhs);
}

template<typename T>
X_INLINE ColorT<T>	ColorT<T>::operator*(T rhs) const
{
	return ColorT<T>(r * rhs, g * rhs, b * rhs, a * rhs);
}


template<typename T>
template<typename OtherT>
X_INLINE ColorT<T>	ColorT<T>::operator*(OtherT rhs) const
{
	return ColorT<T>(r * rhs, g * rhs, b * rhs, a * rhs);
}

template<>
template<>
X_INLINE ColorT<uint8_t> ColorT<uint8_t>::operator*(float rhs) const
{
	return ColorT<uint8_t>(
		static_cast<uint8_t>((float)r * rhs),
		static_cast<uint8_t>((float)g * rhs),
		static_cast<uint8_t>((float)b * rhs),
		static_cast<uint8_t>((float)a * rhs)
	);
}

template<typename T>
X_INLINE ColorT<T>	ColorT<T>::operator/(T rhs) const
{
	return ColorT<T>(r / rhs, g / rhs, b / rhs, a / rhs);
}

template<typename T>
X_INLINE const ColorT<T>&	ColorT<T>::operator+=(T rhs)
{
	r += rhs; g += rhs; b += rhs; a += rhs;
	return *this;
}

template<typename T>
X_INLINE const ColorT<T>&	ColorT<T>::operator-=(T rhs)
{
	r -= rhs; g -= rhs; b -= rhs; a -= rhs;
	return *this;
}

template<typename T>
X_INLINE const ColorT<T>&	ColorT<T>::operator*=(T rhs)
{
	r *= rhs; g *= rhs; b *= rhs; a *= rhs;
	return *this;
}

template<typename T>
X_INLINE const ColorT<T>&	ColorT<T>::operator/=(T rhs)
{
	r /= rhs; g /= rhs; b /= rhs; a /= rhs;
	return *this;
}


template<typename T>
X_INLINE bool ColorT<T>::operator==(const ColorT<T>& rhs) const
{
	return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
}

template<typename T>
X_INLINE bool ColorT<T>::operator != (const ColorT<T>& rhs) const
{
	return !(*this == rhs);
}

template<typename T>
X_INLINE float ColorT<T>::length(void) const
{
	return math<float>::sqrt(static_cast<float>(r*r + g*g + b*b));
}

// tests for zero-length
template<typename T>
X_INLINE ColorT<T>& ColorT<T>::normalize(void)
{
	float s = length();
	if (s > 0.0f) {
		r = static_cast<T>(r / s);
		g = static_cast<T>(g / s);
		b = static_cast<T>(b / s);
	}
	return *this;
}

template<typename T>
X_INLINE ColorT<T> ColorT<T>::premultiplied(void) const
{
	return ColorT<T>(r * a, g * a, b * a, a);
}

template<typename T>
X_INLINE typename CHANTRAIT<T>::Accum ColorT<T>::lengthSquared(void) const
{
	return r * r + g * g + b * b;
}

template<typename T>
X_INLINE ColorT<T> ColorT<T>::lerp(float fact, const ColorT<T> &d) const
{
	return ColorT<T>(
		(T)(r + (d.r - r) * fact),
		(T)(g + (d.g - g) * fact),
		(T)(b + (d.b - b) * fact),
		(T)(a + (d.a - a) * fact));
}

template<typename T>
X_INLINE bool ColorT<T>::compare(const ColorT<T>& oth, const T epsilon)
{
	return math<T>::abs(r - oth.r) <= epsilon &&
		math<T>::abs(g - oth.g) <= epsilon &&
		math<T>::abs(b - oth.b) <= epsilon &&
		math<T>::abs(a - oth.a) <= epsilon;
}

// expose packing util.
template<typename T>
X_INLINE uint8_t ColorT<T>::asRGB332(void)		const
{
	uint8_t cr = CHANTRAIT<uint8_t>::convert(r);
	uint8_t cg = CHANTRAIT<uint8_t>::convert(g);
	uint8_t cb = CHANTRAIT<uint8_t>::convert(b);

	return ((cr >> 5) << 5) | ((cg >> 5) << 2) | (cb >> 5);
}

template<typename T>
X_INLINE uint16_t ColorT<T>::asARGB4444(void)	const
{
	uint8_t cr = CHANTRAIT<uint8_t>::convert(r);
	uint8_t cg = CHANTRAIT<uint8_t>::convert(g);
	uint8_t cb = CHANTRAIT<uint8_t>::convert(b);
	uint8_t ca = CHANTRAIT<uint8_t>::convert(a);

	return ((ca >> 4) << 12) | ((cr >> 4) << 8) | ((cg >> 4) << 4) | (cb >> 4);
}

template<typename T>
X_INLINE uint16_t ColorT<T>::asRGB555(void)		const
{
	uint8_t cr = CHANTRAIT<uint8_t>::convert(r);
	uint8_t cg = CHANTRAIT<uint8_t>::convert(g);
	uint8_t cb = CHANTRAIT<uint8_t>::convert(b);

	return ((cr >> 3) << 10) | ((cg >> 3) << 5) | (cb >> 3);
}

template<typename T>
X_INLINE uint16_t ColorT<T>::asRGB565(void)		const
{
	uint16_t cr = CHANTRAIT<uint8_t>::convert(r);
	uint16_t cg = CHANTRAIT<uint8_t>::convert(g);
	uint16_t cb = CHANTRAIT<uint8_t>::convert(b);

	return ((cr >> 3) << 11) | ((cg >> 2) << 5) | (cb >> 3);
}

template<typename T>
X_INLINE uint32_t ColorT<T>::asBGR888(void)		const
{
	uint8_t cr = CHANTRAIT<uint8_t>::convert(r);
	uint8_t cg = CHANTRAIT<uint8_t>::convert(g);
	uint8_t cb = CHANTRAIT<uint8_t>::convert(b);

	return (cb << 16) | (cg << 8) | cr;
}

template<typename T>
X_INLINE uint32_t ColorT<T>::asRGB888(void)		const
{
	uint8_t cr = CHANTRAIT<uint8_t>::convert(r);
	uint8_t cg = CHANTRAIT<uint8_t>::convert(g);
	uint8_t cb = CHANTRAIT<uint8_t>::convert(b);

	return (cr << 16) | (cg << 8) | cb;
}

template<typename T>
X_INLINE uint32_t ColorT<T>::asABGR8888(void)	const
{
	uint8_t cr = CHANTRAIT<uint8_t>::convert(r);
	uint8_t cg = CHANTRAIT<uint8_t>::convert(g);
	uint8_t cb = CHANTRAIT<uint8_t>::convert(b);
	uint8_t ca = CHANTRAIT<uint8_t>::convert(a);

	return (ca << 24) | (cb << 16) | (cg << 8) | cr;
}

template<typename T>
X_INLINE uint32_t ColorT<T>::asARGB8888(void)	const
{
	uint8_t cr = CHANTRAIT<uint8_t>::convert(r);
	uint8_t cg = CHANTRAIT<uint8_t>::convert(g);
	uint8_t cb = CHANTRAIT<uint8_t>::convert(b);
	uint8_t ca = CHANTRAIT<uint8_t>::convert(a);

	return (ca << 24) | (cr << 16) | (cg << 8) | cb;
}

template<typename T>
X_INLINE void ColorT<T>::shade(const float percent)
{
	r = static_cast<T>(static_cast<float>(r)* (100.f + percent) / 100.f);
	g = static_cast<T>(static_cast<float>(g)* (100.f + percent) / 100.f);
	b = static_cast<T>(static_cast<float>(b)* (100.f + percent) / 100.f);
}
#pragma once

#ifndef X_GUI_WIN_VARS_H_
#define X_GUI_WIN_VARS_H_

X_NAMESPACE_BEGIN(gui)

class XWindow;

class XWinVar 
{
public:
	XWinVar();
	virtual ~XWinVar();

	virtual void Init(const char* _name, XWindow* win) X_ABSTRACT;
	virtual void Set(const char* val) X_ABSTRACT;
	virtual void Update(void) X_ABSTRACT;
	virtual const char* c_str() const X_ABSTRACT;

	X_INLINE XWinVar& operator=(const XWinVar& oth) {
		name = oth.name;
		return *this;
	}

	X_INLINE const char* getName() const {
		if (name) {
			return name;
		}
		return "";
	}
	X_INLINE void SetName(const char *_name) {
		name = _name;
	}

	X_INLINE void SetEval(bool b) {
		eval = b;
	}
	X_INLINE bool GetEval() {
		return eval;
	}

protected:
	core::string name;
	bool eval;
};



class XWinBool : public XWinVar {
public:
	XWinBool() : XWinVar() {};
	~XWinBool()X_OVERRIDE{};

	virtual void Init(const char *_name, XWindow *win) X_OVERRIDE {
		XWinVar::Init(_name, win);
	}
	virtual void Set(const char* val) X_OVERRIDE {
		value = (::atoi(val) != 0);
	}
	virtual void Update() X_OVERRIDE {
		const char* s = getName();
	}
	virtual const char* c_str() const X_OVERRIDE {
		static core::StackString<4> temp;
		temp.clear();
		temp.appendFmt("%i", value);
		return temp.c_str();
	}

	XWinBool& operator=(const XWinBool& oth) {
		XWinVar::operator=(oth);
		value = oth.value;
		return *this;
	}

	X_INLINE operator bool() const { 
		return value; 
	}
	X_INLINE bool operator==(const bool& oth) {
		return (value == oth);
	}
	X_INLINE bool& operator=(const bool& oth) {
		value = oth;
		return value;
	}

protected:
	bool value;
};

class XWinStr : public XWinVar 
{
public:
	XWinStr() : XWinVar() {};
	~XWinStr() X_OVERRIDE{};

	virtual void Init(const char *_name, XWindow *win) X_OVERRIDE{
		XWinVar::Init(_name, win);
	}
	virtual void Set(const char* val) X_OVERRIDE{
		value = val;
	}
	virtual void Update(void) X_OVERRIDE {
		//	const char* s = getName();
	}
	virtual const char* c_str() const X_OVERRIDE {
		return value.c_str();
	}


	X_INLINE XWinStr& operator=(const XWinStr& oth) {
		XWinVar::operator=(oth);
		value = oth.value;
		return *this;
	}
	X_INLINE core::string& operator=(const core::string& oth) {
		value = oth;

		return value;
	}

	X_INLINE bool operator==(const core::string& oth) const {
		return (value == oth);
	}
	X_INLINE bool operator==(const char* oth) const {
		return value.compare(oth);
	}

	X_INLINE operator const char*() const {
		return value.c_str();
	}
	X_INLINE operator const core::string&() const {
		return value;
	}

	X_INLINE size_t getLength(void) {
		return value.length();
	}

protected:
	core::string value;
};


class idWinInt : public XWinVar 
{
public:
	idWinInt() : XWinVar() {};
	~idWinInt() X_OVERRIDE{};

	virtual void Init(const char* _name, XWindow* win) X_OVERRIDE {
		XWinVar::Init(_name, win);
	}
	virtual void Set(const char* val) X_OVERRIDE {
		value = ::atoi(val);;
	}
	virtual void Update(void) X_OVERRIDE{
		const char* s = getName();
	}
	virtual const char* c_str() const X_OVERRIDE {
		static core::StackString<16> temp;
		temp.clear();
		temp.appendFmt("%i", value);
		return temp.c_str();
	}

	idWinInt& operator=(const idWinInt& oth) {
		XWinVar::operator=(oth);
		value = oth.value;
		return *this;
	}
	int& operator=(const int& oth) {
		value = oth;
		return value;
	}
	bool operator==(const int& oth) {
		return (value == oth);
	}
	operator int() const {
		return value;
	}

protected:
	int value;
};

class XWinFloat : public XWinVar 
{
public:
	XWinFloat() : XWinVar() {};
	~XWinFloat() X_OVERRIDE{};

	virtual void Init(const char* _name, XWindow* win) X_OVERRIDE{
		XWinVar::Init(_name, win);
	}
	virtual void Set(const char* val) X_OVERRIDE {
		value = static_cast<float>(::atof(val));
	}
	virtual void Update(void) X_OVERRIDE {
		const char* s = getName();
	}
	virtual const char* c_str() const X_OVERRIDE {
		static core::StackString<32> temp;
		temp.clear();
		temp.appendFmt("%f", value);
		return temp.c_str();
	}

	XWinFloat& operator=(const XWinFloat& oth) {
		XWinVar::operator=(oth);
		value = oth.value;
		return *this;
	}
	float& operator=(const float& oth) {
		value = oth;

		return value;
	}
	bool operator==(const float& oth) {
		return (value == oth);
	}
	operator float() const {
		return value;
	}

protected:
	float value;
};


class XWinVec2 : public XWinVar
{
public:
	XWinVec2() : XWinVar() {};
	~XWinVec2() X_OVERRIDE{};

	virtual void Init(const char* _name, XWindow* win) X_OVERRIDE{
		XWinVar::Init(_name, win);
	}
	virtual void Set(const char* val) X_OVERRIDE{
		// make comma optional.
		if (strchr(val, ',')) 
			sscanf(val, "%f,%f", &value.x, &value.y);
		else 
			sscanf(val, "%f %f", &value.x, &value.y);
	}
	virtual void Update(void) X_OVERRIDE{
		const char* s = getName();
	}
	virtual const char* c_str() const X_OVERRIDE{
		static core::StackString<256> temp;
		temp.clear();
		temp.appendFmt("%f %f", value.x, value.y);
		return temp.c_str();
	}

	XWinVec2& operator=(const XWinVec2& oth) {
		XWinVar::operator=(oth);
		value = oth.value;
		return *this;
	}
	Vec2f& operator=(const Vec2f& oth) {
		value = oth;

		return value;
	}

	bool operator==(const Vec2f& oth) {
		return (value == oth);
	}

	operator const Vec2f&() const {
		return value;
	}
	float x() const {
		return value.x;
	}
	float y() const {
		return value.y;
	}
protected:
	Vec2f value;
};


class XWinVec3 : public XWinVar
{
public:
	XWinVec3() : XWinVar() {};
	~XWinVec3() X_OVERRIDE{};

	virtual void Init(const char* _name, XWindow* win) X_OVERRIDE{
		XWinVar::Init(_name, win);
	}
	virtual void Set(const char* val) X_OVERRIDE{
		// make comma optional.
		if (strchr(val, ','))
		sscanf(val, "%f,%f,%f", &value.x, &value.y, &value.z);
		else
			sscanf(val, "%f %f %f", &value.x, &value.y, &value.z);
	}
	virtual void Update(void) X_OVERRIDE{
		const char* s = getName();
	}
	virtual const char* c_str() const X_OVERRIDE{
		static core::StackString<256> temp;
		temp.clear();
		temp.appendFmt("%f %f %f", value.x, value.y, value.z);
		return temp.c_str();
	}


	XWinVec3& operator=(const XWinVec3& oth) {
		XWinVar::operator=(oth);
		value = oth.value;
		return *this;
	}
	Vec3f& operator=(const Vec3f& oth) {
		value = oth;

		return value;
	}

	bool operator==(const Vec3f& oth) {
		return (value == oth);
	}

	operator const Vec3f&() const {
		return value;
	}

	float x() const {
		return value.x;
	}
	float y() const {
		return value.y;
	}
	float z() const {
		return value.z;
	}
protected:
	Vec3f value;
};


class XWinVec4 : public XWinVar
{
public:
	XWinVec4() : XWinVar() {};
	~XWinVec4() X_OVERRIDE{};

	virtual void Init(const char* _name, XWindow* win) X_OVERRIDE{
		XWinVar::Init(_name, win);
	}
	virtual void Set(const char* val) X_OVERRIDE{
		// make comma optional.
		if (strchr(val, ','))
		sscanf(val, "%f,%f,%f,%f", &value.x, &value.y, &value.z, &value.w);
		else
			sscanf(val, "%f %f %f %f", &value.x, &value.y, &value.z, &value.w);
	}
	virtual void Update(void) X_OVERRIDE{
		const char* s = getName();
	}
	virtual const char* c_str() const X_OVERRIDE{
		static core::StackString<256> temp;
		temp.clear();
		temp.appendFmt("%f %f %f %f", value.x, value.y, value.z, value.w);
		return temp.c_str();
	}


	XWinVec4& operator=(const XWinVec4& oth) {
		XWinVar::operator=(oth);
		value = oth.value;
		return *this;
	}
	Vec4f& operator=(const Vec4f& oth) {
		value = oth;

		return value;
	}

	bool operator==(const Vec4f& oth) {
		return (value == oth);
	}

	operator const Vec4f&() const {
		return value;
	}

	float x() const {
		return value.x;
	}
	float y() const {
		return value.y;
	}
	float z() const {
		return value.z;
	}
	float w() const {
		return value.w;
	}
protected:
	Vec4f value;
};


class XWinRect : public XWinVar
{
public:
	XWinRect() : XWinVar() {};
	~XWinRect() X_OVERRIDE{};

	virtual void Init(const char* _name, XWindow* win) X_OVERRIDE{
		XWinVar::Init(_name, win);
	}
	virtual void Set(const char* val) X_OVERRIDE{
		// make comma optional.
		if (strchr(val, ','))
		sscanf(val, "%f,%f,%f,%f", &value.x1, &value.y1, &value.x2, &value.y2);
		else
			sscanf(val, "%f %f %f %f", &value.x1, &value.y1, &value.x2, &value.y2);
	}
	virtual void Update(void) X_OVERRIDE{
		const char* s = getName();
	}
	virtual const char* c_str() const X_OVERRIDE{
		static core::StackString<256> temp;
		temp.clear();
		temp.appendFmt("%f %f %f %f", value.x1, value.y1, value.x2, value.y2);
		return temp.c_str();
	}


	XWinRect& operator=(const XWinRect& oth) {
		XWinVar::operator=(oth);
		value = oth.value;
		return *this;
	}
	Rectf& operator=(const Rectf& oth) {
		value = oth;

		return value;
	}

//	bool operator==(const Rectf& oth) {
//		return (value == oth);
//	}

	operator const Rectf&() const {
		return value;
	}

	// if want any others, just get the rect object above.
	float x1() const {
		return value.getX1();
	}
	float y1() const {
		return value.getY1();
	}
	float x2() const {
		return value.getX2();
	}
	float y2() const {
		return value.getY2();
	}

	float width(void) const {
		return value.getWidth();
	}
	float height(void) const {
		return value.getHeight();
	}

protected:
	Rectf value;
};

class XWinColor : public XWinVar
{
public:
	XWinColor() : XWinVar() {};
	~XWinColor() X_OVERRIDE{};

	virtual void Init(const char* _name, XWindow* win) X_OVERRIDE{
		XWinVar::Init(_name, win);
	}
	virtual void Set(const char* val) X_OVERRIDE{
		// make comma optional.
		if (strchr(val, ','))
		sscanf(val, "%f,%f,%f,%f", &value.r, &value.g, &value.b, &value.a);
		else
			sscanf(val, "%f %f %f %f", &value.r, &value.g, &value.b, &value.a);
	}
	virtual void Update(void) X_OVERRIDE{
		const char* s = getName();
	}
	virtual const char* c_str() const X_OVERRIDE{
		static core::StackString<256> temp;
		temp.clear();
		temp.appendFmt("%f %f %f %f", value.r, value.g, value.b, value.a);
		return temp.c_str();
	}


	XWinColor& operator=(const XWinColor& oth) {
		XWinVar::operator=(oth);
		value = oth.value;
		return *this;
	}

	Color& operator=(const Color& oth) {
		value = oth;
		return value;
	}

	bool operator==(const Color& oth) {
		return (value == oth);
	}

	operator const Color&() const {
		return value;
	}

protected:
	Color value;
};






X_NAMESPACE_END

#endif // !X_GUI_WIN_VARS_H_
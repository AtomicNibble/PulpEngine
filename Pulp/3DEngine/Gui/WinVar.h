#pragma once

#ifndef X_GUI_WIN_VARS_H_
#define X_GUI_WIN_VARS_H_

#include "RegExp.h"

X_NAMESPACE_BEGIN(engine)

namespace gui
{

	class XWindow;

	// same
	typedef RegisterType VarType;

	class XWinVar
	{
	public:
		XWinVar();
		virtual ~XWinVar();

		virtual void Init(const char* _name, XWindow* win) X_ABSTRACT;
		virtual void Set(const char* val) X_ABSTRACT;
		virtual void Update(void) X_ABSTRACT;
		virtual const char* c_str() const X_ABSTRACT;
		virtual VarType::Enum getType(void) X_ABSTRACT;

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

		virtual void fromFile(core::XFile* pFile) X_ABSTRACT;
		virtual void toFile(core::XFile* pFile) X_ABSTRACT;

	protected:
		core::string name;
		bool eval;
	};



	class XWinBool : public XWinVar {
	public:
		XWinBool() : value(false), XWinVar() {};
		~XWinBool()X_OVERRIDE {};

		virtual void Init(const char *_name, XWindow *win) X_OVERRIDE {
			XWinVar::Init(_name, win);
		}
		virtual void Set(const char* val) X_OVERRIDE {
			value = (::atoi(val) != 0);
		}
		virtual void Update() X_OVERRIDE {
			//		const char* s = getName();
		}
		virtual const char* c_str() const X_OVERRIDE {
			static core::StackString<4> temp;
			temp.clear();
			temp.appendFmt("%i", value);
			return temp.c_str();
		}
		virtual VarType::Enum getType(void) X_OVERRIDE {
			return VarType::BOOL;
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

		virtual void fromFile(core::XFile* pFile) X_OVERRIDE;
		virtual void toFile(core::XFile* pFile) X_OVERRIDE;

	protected:
		bool value;
	};

	class XWinStr : public XWinVar
	{
	public:
		XWinStr() : XWinVar() {};
		~XWinStr() X_OVERRIDE {};

		virtual void Init(const char *_name, XWindow *win) X_OVERRIDE {
			XWinVar::Init(_name, win);
		}
		virtual void Set(const char* val) X_OVERRIDE {
			value = val;
		}
		virtual void Update(void) X_OVERRIDE {
			//	const char* s = getName();
		}
		virtual const char* c_str() const X_OVERRIDE {
			return value.c_str();
		}
		virtual VarType::Enum getType(void) X_OVERRIDE {
			return VarType::STRING;
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

		X_INLINE size_t getLength(void) const {
			return value.length();
		}

		X_INLINE auto begin(void) const {
			return value.begin();
		}
		X_INLINE auto end(void) const {
			return value.end();
		}


		virtual void fromFile(core::XFile* pFile) X_OVERRIDE;
		virtual void toFile(core::XFile* pFile) X_OVERRIDE;

	protected:
		core::string value;
	};


	class XWinInt : public XWinVar
	{
	public:
		XWinInt() : value(0), XWinVar() {};
		~XWinInt() X_OVERRIDE {};

		virtual void Init(const char* _name, XWindow* win) X_OVERRIDE {
			XWinVar::Init(_name, win);
		}
		virtual void Set(const char* val) X_OVERRIDE {
			value = ::atoi(val);;
		}
		virtual void Update(void) X_OVERRIDE {
			//		const char* s = getName();
		}
		virtual const char* c_str() const X_OVERRIDE {
			static core::StackString<16> temp;
			temp.clear();
			temp.appendFmt("%i", value);
			return temp.c_str();
		}
		virtual VarType::Enum getType(void) X_OVERRIDE {
			return VarType::INT;
		}

		XWinInt& operator=(const XWinInt& oth) {
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

		virtual void fromFile(core::XFile* pFile) X_OVERRIDE;
		virtual void toFile(core::XFile* pFile) X_OVERRIDE;

	protected:
		int value;
	};

	class XWinFloat : public XWinVar
	{
	public:
		XWinFloat() : value(0.f), XWinVar() {};
		~XWinFloat() X_OVERRIDE {};

		virtual void Init(const char* _name, XWindow* win) X_OVERRIDE {
			XWinVar::Init(_name, win);
		}
		virtual void Set(const char* val) X_OVERRIDE {
			value = core::strUtil::StringToFloat<float>(val);
		}
		virtual void Update(void) X_OVERRIDE {
			//		const char* s = getName();
		}
		virtual const char* c_str() const X_OVERRIDE {
			static core::StackString<32> temp;
			temp.clear();
			temp.appendFmt("%f", value);
			return temp.c_str();
		}
		virtual VarType::Enum getType(void) X_OVERRIDE {
			return VarType::FLOAT;
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

		virtual void fromFile(core::XFile* pFile) X_OVERRIDE;
		virtual void toFile(core::XFile* pFile) X_OVERRIDE;

	protected:
		float value;
	};


	class XWinVec2 : public XWinVar
	{
	public:
		XWinVec2() : XWinVar() {};
		~XWinVec2() X_OVERRIDE {};

		virtual void Init(const char* _name, XWindow* win) X_OVERRIDE {
			XWinVar::Init(_name, win);
		}
		virtual void Set(const char* val) X_OVERRIDE {
			// make comma optional.
			if (strchr(val, ','))
				sscanf(val, "%f,%f", &value.x, &value.y);
			else
				sscanf(val, "%f %f", &value.x, &value.y);
		}
		virtual void Update(void) X_OVERRIDE {
			//		const char* s = getName();
		}
		virtual const char* c_str() const X_OVERRIDE {
			static core::StackString<256> temp;
			temp.clear();
			temp.appendFmt("%f %f", value.x, value.y);
			return temp.c_str();
		}
		virtual VarType::Enum getType(void) X_OVERRIDE {
			return VarType::VEC2;
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

		virtual void fromFile(core::XFile* pFile) X_OVERRIDE;
		virtual void toFile(core::XFile* pFile) X_OVERRIDE;

	protected:
		Vec2f value;
	};


	class XWinVec3 : public XWinVar
	{
	public:
		XWinVec3() : XWinVar() {};
		~XWinVec3() X_OVERRIDE {};

		virtual void Init(const char* _name, XWindow* win) X_OVERRIDE {
			XWinVar::Init(_name, win);
		}
		virtual void Set(const char* val) X_OVERRIDE {
			// make comma optional.
			if (strchr(val, ','))
				sscanf(val, "%f,%f,%f", &value.x, &value.y, &value.z);
			else
				sscanf(val, "%f %f %f", &value.x, &value.y, &value.z);
		}
		virtual void Update(void) X_OVERRIDE {
			//		const char* s = getName();
		}
		virtual const char* c_str() const X_OVERRIDE {
			static core::StackString<256> temp;
			temp.clear();
			temp.appendFmt("%f %f %f", value.x, value.y, value.z);
			return temp.c_str();
		}
		virtual VarType::Enum getType(void) X_OVERRIDE {
			return VarType::VEC3;
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

		virtual void fromFile(core::XFile* pFile) X_OVERRIDE;
		virtual void toFile(core::XFile* pFile) X_OVERRIDE;

	protected:
		Vec3f value;
	};


	class XWinVec4 : public XWinVar
	{
	public:
		XWinVec4() : XWinVar() {};
		~XWinVec4() X_OVERRIDE {};

		virtual void Init(const char* _name, XWindow* win) X_OVERRIDE {
			XWinVar::Init(_name, win);
		}
		virtual void Set(const char* val) X_OVERRIDE {
			// make comma optional.
			if (strchr(val, ','))
				sscanf(val, "%f,%f,%f,%f", &value.x, &value.y, &value.z, &value.w);
			else
				sscanf(val, "%f %f %f %f", &value.x, &value.y, &value.z, &value.w);
		}
		virtual void Update(void) X_OVERRIDE {
			//		const char* s = getName();
		}
		virtual const char* c_str() const X_OVERRIDE {
			static core::StackString<256> temp;
			temp.clear();
			temp.appendFmt("%f %f %f %f", value.x, value.y, value.z, value.w);
			return temp.c_str();
		}
		virtual VarType::Enum getType(void) X_OVERRIDE {
			return VarType::VEC4;
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
		Vec4f& getVec(void) {
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

		virtual void fromFile(core::XFile* pFile) X_OVERRIDE;
		virtual void toFile(core::XFile* pFile) X_OVERRIDE;

	protected:
		Vec4f value;
	};


	class XWinRect : public XWinVar
	{
	public:
		XWinRect() : XWinVar() {};
		~XWinRect() X_OVERRIDE {};

		virtual void Init(const char* _name, XWindow* win) X_OVERRIDE {
			XWinVar::Init(_name, win);
		}
		virtual void Set(const char* val) X_OVERRIDE {
			// make comma optional.
			if (strchr(val, ','))
				sscanf(val, "%f,%f,%f,%f", &value.x1, &value.y1, &value.x2, &value.y2);
			else
				sscanf(val, "%f %f %f %f", &value.x1, &value.y1, &value.x2, &value.y2);

			// turn from width / height into positions.
			value.x2 = value.x1 + value.x2;
			value.y2 = value.y1 + value.y2;

		}
		void Set(const Vec4f& oth) {
			value.x1 = oth[0];
			value.y1 = oth[1];
			value.x2 = oth[2];
			value.y2 = oth[3];
		}
		virtual void Update(void) X_OVERRIDE {
			//		const char* s = getName();
		}
		virtual const char* c_str() const X_OVERRIDE {
			static core::StackString<256> temp;
			temp.clear();
			temp.appendFmt("%f %f %f %f", value.x1, value.y1, value.x2, value.y2);
			return temp.c_str();
		}
		virtual VarType::Enum getType(void) X_OVERRIDE {
			return VarType::RECT;
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
		Vec4f asVec4(void) const {
			return Vec4f(value.x1, value.y1, value.x2, value.y2);
		}

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

		virtual void fromFile(core::XFile* pFile) X_OVERRIDE;
		virtual void toFile(core::XFile* pFile) X_OVERRIDE;

	protected:
		Rectf value;
	};

	class XWinColor : public XWinVar
	{
	public:
		XWinColor() : XWinVar() {};
		~XWinColor() X_OVERRIDE {};

		virtual void Init(const char* _name, XWindow* win) X_OVERRIDE {
			XWinVar::Init(_name, win);
		}
		virtual void Set(const char* val) X_OVERRIDE {
			
			// TODO: use Color::fromString
			Color col;
			// make comma optional.
			if (strchr(val, ',')) {
				sscanf(val, "%f,%f,%f,%f", &col.r, &col.g, &col.b, &col.a);
			}
			else {
				sscanf(val, "%f %f %f %f", &col.r, &col.g, &col.b, &col.a);
			}

			value = col;
		}
		virtual void Update(void) X_OVERRIDE {
			//		const char* s = getName();
		}
		virtual const char* c_str() const X_OVERRIDE {
			// TODO make thread safe?
			static core::StackString<256> temp;
			temp.clear();
			temp.appendFmt("%f %f %f %f", value.r, value.g, value.b, value.a);
			return temp.c_str();
		}
		virtual VarType::Enum getType(void) X_OVERRIDE {
			return VarType::COLOR;
		}
		virtual void Set(const Vec4f& oth) {
			value = Color(oth);
		}

		XWinColor& operator=(const XWinColor& oth) {
			XWinVar::operator=(oth);
			value = oth.value;
			return *this;
		}

		Color8u& operator=(const Color8u& oth) {
			value = oth;
			return value;
		}

		bool operator==(const Color8u& oth) {
			return (value == oth);
		}

		operator const Color8u&() const {
			return value;
		}

		float r() const {
			return value.r;
		}
		float g() const {
			return value.g;
		}
		float b() const {
			return value.b;
		}
		float a() const {
			return value.a;
		}

		virtual void fromFile(core::XFile* pFile) X_OVERRIDE;
		virtual void toFile(core::XFile* pFile) X_OVERRIDE;

	protected:
		Color8u value;
	};




} // namespace gui

X_NAMESPACE_END

#endif // !X_GUI_WIN_VARS_H_
#include "stdafx.h"
#include "RegExp.h"

#include "WinVar.h"
#include "XWindow.h"

X_NAMESPACE_BEGIN(engine)

namespace gui
{

	int XRegister::REGCOUNT[RegisterType::ENUM_COUNT] = {
		1, // BOOL
		1, // INT
		1, // FLOAT
		2, // VEC2
		3, // VEC3
		4, // VEC4
		4, // COLOR
		4, // RECT
		0  // STRING
	};


	void XRegister::SetToRegs(float* registers)
	{
		int i;
		Vec4f v;

		if (!enabled_ || var_ == nullptr) {
			return;
		}

		switch (type_)
		{
			case RegisterType::BOOL:
				v[0] = *static_cast<XWinBool*>(var_);
				break;

			case RegisterType::INT:
				v[0] = static_cast<float>(*static_cast<XWinInt*>(var_));
				break;

			case RegisterType::FLOAT:
				v[0] = *static_cast<XWinFloat*>(var_);
				break;

			case RegisterType::VEC2:
			{
				Vec2f v2 = *static_cast<XWinVec2*>(var_);
				v[0] = v2[0];
				v[1] = v2[1];
				break;
			}

			case RegisterType::VEC3:
			{
				Vec3f v3 = *static_cast<XWinVec3*>(var_);
				v[0] = v3[0];
				v[1] = v3[1];
				v[2] = v3[2];
				break;
			}

			case RegisterType::VEC4:
				v = *static_cast<XWinVec4*>(var_);
				break;

			case RegisterType::RECT:
			{
				Rectf rect = *static_cast<XWinRect*>(var_);
				v.x = rect.x1;
				v.y = rect.y1;
				v.z = rect.x2;
				v.w = rect.y2;
				break;
			}

			case RegisterType::COLOR:
			{
				Color8u color = *static_cast<XWinColor*>(var_);
				auto colF = Color(color);
				v.x = colF.r;
				v.y = colF.g;
				v.z = colF.b;
				v.w = colF.a;
				break;
			}

#if X_DEBUG
			default:
				X_ASSERT_UNREACHABLE();
				break;
#else
				X_NO_SWITCH_DEFAULT;
#endif // !X_DEBUG
		}

		for (i = 0; i < regCount_; i++) {
			registers[regs_[i]] = v[i];
		}
	}

	void XRegister::GetFromRegs(float* registers)
	{
		Vec4f v;
		Rectf rect;
		Color color;

		if (!enabled_ || var_ == nullptr) {
			return;
		}

		for (int i = 0; i < regCount_; i++) {
			v[i] = registers[regs_[i]];
		}

		switch (type_)
		{
			case RegisterType::BOOL:
				*static_cast<XWinBool*>(var_) = (v[0] != 0.0f);
				break;

			case RegisterType::INT:
				*static_cast<XWinInt*>(var_) = static_cast<int>(v[0]);
				break;

			case RegisterType::FLOAT:
				*static_cast<XWinFloat*>(var_) = v[0];
				break;

			case RegisterType::VEC2:
				*static_cast<XWinVec2*>(var_) = v.xy();
				break;

			case RegisterType::VEC3:
				*static_cast<XWinVec3*>(var_) = v.xyz();
				break;

			case RegisterType::VEC4:
				*static_cast<XWinVec4*>(var_) = v;
				break;

			case RegisterType::RECT:
				rect.x1 = v.x;
				rect.y1 = v.y;
				rect.x2 = v.z + v.x;
				rect.y2 = v.w + v.y;
				*static_cast<XWinRect*>(var_) = rect;
				break;

			case RegisterType::COLOR:
				color.r = v.x;
				color.g = v.y;
				color.b = v.z;
				color.a = v.w;
				*static_cast<XWinColor*>(var_) = color;
				break;

#if X_DEBUG
			default:
				X_ASSERT_UNREACHABLE();
				break;
#else
				X_NO_SWITCH_DEFAULT;
#endif // !X_DEBUG		
		}
	}


	// ====================================

	XRegisterList::XRegisterList() :
		registers_(g_3dEngineArena),
		regHash_(g_3dEngineArena)
	{

	}

	XRegisterList::~XRegisterList()
	{

	}


	void XRegisterList::AddReg(const char* name, RegisterType::Enum type, Vec4f data,
		XWindow* win, XWinVar* var)
	{
		X_UNUSED(data);
		X_UNUSED(win);

		if (FindReg(name) == nullptr)
		{
			int numRegs = XRegister::REGCOUNT[type];

			XRegister* reg = X_NEW(XRegister, g_3dEngineArena, "Reg")(name, type);
			reg->var_ = var;

			for (int i = 0; i < numRegs; i++) {
				reg->regs_[i] = 0; // win->ExpressionConstant(data[i]);
			}

			int hash = regHash_.generateKey(name, false);
			regHash_.add(hash, safe_static_cast<int, size_t>(registers_.append(reg)));
		}
	}

	void XRegisterList::AddReg(const char* name, RegisterType::Enum type, core::XParser& lex,
		XWindow* win, XWinVar* var)
	{
		XRegister* reg;

		reg = FindReg(name);

		if (reg == nullptr)
		{
			int numRegs = XRegister::REGCOUNT[type];

			reg = X_NEW(XRegister, g_3dEngineArena, "Reg")(name, type);
			reg->var_ = var;

			if (type == RegisterType::STRING)
			{
				core::XLexToken tok;
				if (lex.ReadToken(tok))
				{
					core::StackString512 Lname(tok.begin(), tok.end());
					var->Init(Lname.c_str(), win);
				}
			}
			else
			{
				for (int i = 0; i < numRegs; i++)
				{
					reg->regs_[i] = safe_static_cast<uint16_t, int>(win->ParseExpression(lex, NULL));
				}
			}

			int hash = regHash_.generateKey(name, false);
			regHash_.add(hash, safe_static_cast<int, size_t>(registers_.append(reg)));
		}
	}

	XRegister* XRegisterList::FindReg(const char* name)
	{
		int hash = regHash_.generateKey(name, false);
		for (int i = regHash_.first(hash); i != -1; i = regHash_.next(i)) {
			if (registers_[i]->name_ == name) {
				return registers_[i];
			}
		}
		return nullptr;
	}

	void XRegisterList::SetToRegs(float* registers)
	{
		for (size_t i = 0; i < registers_.size(); i++) {
			registers_[i]->SetToRegs(registers);
		}
	}

	void XRegisterList::GetFromRegs(float* registers)
	{
		for (size_t i = 0; i < registers_.size(); i++) {
			registers_[i]->GetFromRegs(registers);
		}
	}

	void XRegisterList::Reset()
	{
		registers_.free();
		regHash_.clear();
	}

} // namespace gui

X_NAMESPACE_END
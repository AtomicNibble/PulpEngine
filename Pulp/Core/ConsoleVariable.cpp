#include "stdafx.h"

// #include <String\xStr.h>

#include "Console.h"
#include "ConsoleVariable.h"

#include <IConsole.h>

#include "String\Lexer.h"

X_NAMESPACE_BEGIN(core)

template class CVarInt<CVarBaseConst>;
template class CVarInt<CVarBaseHeap>;

namespace
{

	inline uint32_t AlphaBit(char c)
	{
		return c >= 'a' && c <= 'z' ? 1 << (c - 'z' + 31) : 0;
	}

	inline int TextToInt(const char* s, int nCurrent, bool bBitfield)
	{
		int nValue = 0;
		if (s)
		{
			char* e;
			if (bBitfield)
			{
				// Bit manipulation.
				if (*s == '^')
					// Bit number
					nValue = 1 << strtol(++s, &e, 10);
				else
					// Full number
					nValue = strtol(s, &e, 10);

				// Check letter codes.
				for (; *e >= 'a'&& *e <= 'z'; e++)
					nValue |= AlphaBit(*e);

				if (*e == '+')
					nValue = nCurrent | nValue;
				else if (*e == '-')
					nValue = nCurrent & ~nValue;
				else if (*e == '^')
					nValue = nCurrent ^ nValue;
			}
			else
				nValue = strtol(s, &e, 10);
		}
		return nValue;
	}


} // namespace 

CVarBase::CVarBase(XConsole* pConsole, int nFlags, const char* desc) :
Desc_(desc),
Flags_(nFlags),
pConsole_(pConsole)
{
	//	Name_ = sName;
}

CVarBase::~CVarBase()
{

}

void CVarBase::ForceSet(const char* s)
{
	X_UNUSED(s);
}

void CVarBase::SetDefault(const char* s)
{
	X_UNUSED(s);
}

ICVar::FlagType CVarBase::GetFlags() const
{
	return Flags_;
}

ICVar::FlagType CVarBase::SetFlags(FlagType flags)
{
	Flags_ = flags;
	return Flags_;
}

void CVarBase::SetModified(void)
{
	Flags_.Set(VarFlag::MODIFIED);
}
//const char* CVarBase::GetName() const
//{
//	return Name_;
//}

const char* CVarBase::GetDesc() const
{
	return Desc_;
}


void CVarBase::Release()
{
	this->pConsole_->UnregisterVariable(GetName());
}

ICVar* CVarBase::SetOnChangeCallback(ConsoleVarFunc changeFunc)
{
	changeFunc_ = changeFunc;
	return this;
}

ConsoleVarFunc CVarBase::GetOnChangeCallback()
{
	return changeFunc_;
}

void CVarBase::OnModified()
{
	Flags_.Set(VarFlag::MODIFIED);
}


void CVarBase::Reset()
{
	// nothing to set here :D
}


// ========================================================


template<class T>
void CVarInt<T>::Set(const char* s)
{
	int nValue = TextToInt(s, IntValue_, Flags_.IsSet(VarFlag::BITFIELD));

	Set(nValue);
}


// ========================================================

void CVarIntRef::Set(const char* s)
{
	int nValue = TextToInt(s, IntValue_, Flags_.IsSet(VarFlag::BITFIELD));

	Set(nValue);
}

// ========================================================


bool CVarColRef::ColorFromString(const char* pStr, Color& out, bool Slient)
{
	Color col;
	int i;

	core::XLexer lex(pStr, pStr + strlen(pStr));
	core::XLexToken token;

	for (i = 0; i < 4; i++)
	{
		if (lex.ReadToken(token))
		{
			if (token.GetType() == TokenType::NUMBER)// && core::bitUtil::IsBitFlagSet(token.subtype,TT_FLOAT))
			{
				col[i] = token.GetFloatValue();
			}
			else
			{
				X_ERROR_IF(!Slient,"Cvar", "failed to set color, invalid input");
				return false;
			}
		}
		else
		{
			if (i == 1)
			{
				// we allow all 4 colors to be set with 1 color.
				// could cap here for a saving if you really wish :P
				col.g = col.r;
				col.b = col.r;
				col.a = col.r;
				break;
			}
			if (i == 3)
			{
				// solid alpha.
				col.a = 1.f;
			}
			else
			{
				X_ERROR_IF(!Slient, "Cvar", "failed to set color, require either 1 or 4 real numbers");
				return false;
			}
		}
	}


	// cap any values.
	col.r = core::Min(1.f, core::Max(0.f, col.r));
	col.g = core::Min(1.f, core::Max(0.f, col.g));
	col.b = core::Min(1.f, core::Max(0.f, col.b));
	col.a = core::Min(1.f, core::Max(0.f, col.a));

	out = col; 
	return true;
}

void CVarColRef::Set(const char* s)
{
	X_ASSERT_NOT_NULL(s);

	if (Flags_.IsSet(VarFlag::READONLY))
		return;

	Color col;

	if (!ColorFromString(s, col, false))
		return;

	// any diffrent?
	if (ColValue_.compare(col, 0.001f))
		return;

	// assign
	ColValue_ = col;

	OnModified();

	if (changeFunc_) {
		changeFunc_.Invoke(this); // change callback.
	}
}


// ========================================================

bool CVarVec3Ref::Vec3FromString(const char* pStr, Vec3f& out, bool Slient)
{
	Vec3f vec;
	int i;

	core::XLexer lex(pStr, pStr + strlen(pStr));
	core::XLexToken token;

	for (i = 0; i < 3; i++)
	{
		if (lex.ReadToken(token))
		{
			if (token.GetType() == TokenType::NUMBER)// && core::bitUtil::IsBitFlagSet(token.subtype,TT_FLOAT))
			{
				vec[i] = token.GetFloatValue();
			}
			else
			{
				X_ERROR_IF(!Slient, "CVar", "failed to set vec3, invalid input");
				return false;
			}
		}
		else
		{
			if (i == 1)
			{
				// we allow all 3 values to be set with 1 val.
				vec.y = vec.x;
				vec.z = vec.x;
				break;
			}
			else
			{
				X_ERROR_IF(!Slient, "CVar", "failed to set vec3, require either 1 or 3 real numbers");
				return false;
			}
		}
	}


	out = vec;
	return true;
}

void CVarVec3Ref::Set(const char* s)
{
	X_ASSERT_NOT_NULL(s);

	if (Flags_.IsSet(VarFlag::READONLY))
		return;

	Vec3f vec;

	if (!Vec3FromString(s, vec, false))
		return;

	// any diffrent?
	if (Value_.compare(vec, 0.001f))
		return;

	// assign
	Value_ = vec;

	OnModified();

	if (changeFunc_) {
		changeFunc_.Invoke(this); // change callback.	
	}
}


X_NAMESPACE_END
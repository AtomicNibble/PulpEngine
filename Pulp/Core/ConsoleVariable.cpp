#include "stdafx.h"

// #include <String\xStr.h>
#include <String\StackString.h>

#include "Console.h"
#include "ConsoleVariable.h"

#include <IConsole.h>

#include "String\Lexer.h"

X_NAMESPACE_BEGIN(core)


CVarBase::CVarBase(XConsole* pConsole, int nFlags, const char* desc) :
Desc_(desc),
Flags_(nFlags),
pChangeFunc_(nullptr),
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

void CVarBase::SetOnChangeCallback(ConsoleVarFunc pChangeFunc)
{
	pChangeFunc_ = pChangeFunc;
}

ConsoleVarFunc CVarBase::GetOnChangeCallback()
{
	return pChangeFunc_;
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

	if (pChangeFunc_)
		pChangeFunc_(this); // change callback.	
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

	if (pChangeFunc_)
		pChangeFunc_(this); // change callback.	
}


X_NAMESPACE_END
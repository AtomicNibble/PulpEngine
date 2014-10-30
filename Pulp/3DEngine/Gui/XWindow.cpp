#include "stdafx.h"
#include "XWindow.h"

#include <IFont.h>

X_NAMESPACE_BEGIN(gui)

namespace
{
	static const Color g_DefaultColor_Back(1, 1, 1, 1);
	static const Color g_DefaultColor_Fore(1, 1, 1, 1);
	static const Color g_DefaultColor_Hover(1, 1, 1, 1);
	static const Color g_DefaultColor_Border(1, 1, 1, 1);

	static const float g_DefaultTextScale = 0.35f;
}

XRegEntry XWindow::s_RegisterVars[] =
{
	{ "rect", RegisterType::RECT },

	{ "backcolor", RegisterType::COLOR },
	{ "forecolor", RegisterType::COLOR },
	{ "hovercolor", RegisterType::COLOR },
	{ "bordercolor", RegisterType::COLOR },

	{ "visible", RegisterType::BOOL },
	{ "hidecursor", RegisterType::BOOL },

	{ "textscale", RegisterType::FLOAT },
	{ "text", RegisterType::STRING },
};


const char* XWindow::s_ScriptNames[XWindow::ScriptFunction::ENUM_COUNT] = {
	"onMouseEnter",
	"onMouseExit",
	"onEsc",
	"onEnter",
	"onOpen",
	"onClose"
};


const int XWindow::s_NumRegisterVars = sizeof(s_RegisterVars) / sizeof(const char*);

bool XWindow::s_registerIsTemporary[MAX_EXPRESSION_REGISTERS]; 


XWindow::XWindow() :
children_(g_3dEngineArena),
expressionRegisters_(g_3dEngineArena)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pFont);

	// reg's
	backColor_ = g_DefaultColor_Back;
	foreColor_ = g_DefaultColor_Fore;
	hoverColor_ = g_DefaultColor_Hover;
	borderColor_ = g_DefaultColor_Border;
	visable_ = true;
	hideCursor_ = false;
	textScale_ = g_DefaultTextScale;
	// ~

	borderSize_ = 1.f;
	textAlignX_ = 0.f;
	textAlignY_ = 0.f;

	textAlign_ = TextAlign::LEFT;

	shadowText_ = false;

	childId_ = 0;

	pParent_ = nullptr;
	pFocusedChild_ = nullptr;
	pCaptureChild_ = nullptr;
	pOverChild_ = nullptr;

	// default font
	pFont_ = gEnv->pFont->GetFont("default");

	hover_ = false;
}

XWindow::~XWindow()
{

}




// ----------------------------------------

// Drawing
void XWindow::reDraw(void)
{

}

void XWindow::drawDebug(void)
{

}

void XWindow::drawCaption(void)
{

}


// -------------- Parsing ---------------
void XWindow::SaveExpressionParseState()
{
	pSaveTemps_ = (bool*)X_NEW_ARRAY(bool, MAX_EXPRESSION_REGISTERS, g_3dEngineArena, "ParseState");
	memcpy(pSaveTemps_, s_registerIsTemporary, MAX_EXPRESSION_REGISTERS * sizeof(bool));
}

void XWindow::RestoreExpressionParseState()
{
	memcpy(s_registerIsTemporary, pSaveTemps_, MAX_EXPRESSION_REGISTERS * sizeof(bool));
	X_DELETE_ARRAY(pSaveTemps_,g_3dEngineArena);
}


bool XWindow::Parse(core::XLexer& lex)
{
	// we have a { }
	core::XLexToken token;

	if (!lex.ExpectTokenString("{"))
		return false;

	if (!lex.ReadToken(token))
		return false;

	// read what we got in the brace baby.
	while (!token.isEqual("}"))
	{
		if (token.isEqual("itemDef"))
		{
			XWindow* win = X_NEW(XWindow,g_3dEngineArena,"ItemDef");
			win->setParent(this);

			SaveExpressionParseState();
				win->Parse(lex);
			RestoreExpressionParseState();

			addChild(win);
		//	SetFocus(win, false);

		}
		else if (ParseVar(token, lex))
		{
			// it's a var :)
		}
		else if (ParseRegEntry(token, lex))
		{
			// it's a registry var.
		}
		else if (ParseScriptFunction(token, lex))
		{
			// it's a function definition.
		}


		if (!lex.ReadToken(token))
		{
			X_ERROR("Gui", "error while parsing windowDef");
			return false;
		}
	}


	EvalRegs(-1, true);

	return true;
}

void XWindow::EvaluateRegisters(float* registers) 
{
	size_t i;
	Vec4f v;

	size_t erc = expressionRegisters_.size();

	// copy the constants
	for (i = WEXP_REG_NUM_PREDEFINED; i < erc; i++) {
		registers[i] = expressionRegisters_[i];
	}
}

float XWindow::EvalRegs(int test, bool force) 
{
	static float regs[MAX_EXPRESSION_REGISTERS];
	static XWindow* lastEval = NULL;

	if (!force && test >= 0 && test < MAX_EXPRESSION_REGISTERS && lastEval == this) {
		return regs[test];
	}

	lastEval = this;

	if (expressionRegisters_.size()) {
		regList_.SetToRegs(regs);
		EvaluateRegisters(regs);
		regList_.GetFromRegs(regs);
	}

	if (test >= 0 && test < MAX_EXPRESSION_REGISTERS) {
		return regs[test];
	}

	return 0.0;
}



int XWindow::ExpressionConstant(float f)
{
	int	i;

	for (i = WEXP_REG_NUM_PREDEFINED; i < expressionRegisters_.size(); i++)
	{
		if (!s_registerIsTemporary[i] && expressionRegisters_[i] == f) {
			return i;
		}
	}

	if (expressionRegisters_.size() == MAX_EXPRESSION_REGISTERS) {
	//	common->Warning("expressionConstant: gui %s hit MAX_EXPRESSION_REGISTERS", gui->GetSourceFile());
		return 0;
	}

	int c = safe_static_cast<int, size_t>(expressionRegisters_.size());
	if (i > c)
	{
		while (i > c) 
		{
			expressionRegisters_.append(-9999999);
			i--;
		}
	}

	i = safe_static_cast<int,size_t>(expressionRegisters_.append(f));
	s_registerIsTemporary[i] = false;
	return i;
}

int XWindow::ParseTerm(core::XLexer& lex, XWinVar* var, int component)
{
	core::XLexToken token;
//	int	a, b;

	lex.ReadToken(token);

	/*
	if (token.isEqual("("))
	{
		a = ParseExpression(lex);
		src->ExpectTokenString(")");
		return a;
	}
	*/

	if (token.isEqual("time")) {
		return WEXP_REG_TIME;
	}

	// parse negative numbers
	if (token.isEqual("-")) 
	{
		lex.ReadToken(token);
		if (token.type == TT_NUMBER || token.isEqual("."))
		{
			return ExpressionConstant(-(float)token.GetFloatValue());
		}
	//	src->Warning("Bad negative number '%s'", token.c_str());
		return 0;
	}

	if (token.type == TT_NUMBER || token.isEqual(".") || token.isEqual("-")) {
		return ExpressionConstant((float)token.GetFloatValue());
	}

	/*
	// see if it is a table name
	const idDeclTable *table = static_cast<const idDeclTable *>(declManager->FindType(DECL_TABLE, token.c_str(), false));
	if (table) {
		a = table->Index();
		// parse a table expression
		src->ExpectTokenString("[");
		b = ParseExpression(src);
		src->ExpectTokenString("]");
		return EmitOp(a, b, WOP_TYPE_TABLE);
	}

	if (var == NULL) {
		var = GetWinVarByName(token, true);
	}
	if (var) {
		a = (int)var;
		//assert(dynamic_cast<idWinVec4*>(var));
		var->Init(token, this);
		b = component;
		if (dynamic_cast<idWinVec4*>(var)) {
			if (src->ReadToken(&token)) {
				if (token == "[") {
					b = ParseExpression(src);
					src->ExpectTokenString("]");
				}
				else {
					src->UnreadToken(&token);
				}
			}
			return EmitOp(a, b, WOP_TYPE_VAR);
		}
		else if (dynamic_cast<idWinFloat*>(var)) {
			return EmitOp(a, b, WOP_TYPE_VARF);
		}
		else if (dynamic_cast<idWinInt*>(var)) {
			return EmitOp(a, b, WOP_TYPE_VARI);
		}
		else if (dynamic_cast<idWinBool*>(var)) {
			return EmitOp(a, b, WOP_TYPE_VARB);
		}
		else if (dynamic_cast<idWinStr*>(var)) {
			return EmitOp(a, b, WOP_TYPE_VARS);
		}
		else {
			src->Warning("Var expression not vec4, float or int '%s'", token.c_str());
		}
		return 0;
	}
	else {
		// ugly but used for post parsing to fixup named vars
		char *p = new (TAG_OLD_UI) char[token.Length() + 1];
		strcpy(p, token);
		a = (int)p;
		b = -2;
		return EmitOp(a, b, WOP_TYPE_VAR);
	}
	*/

	return 0;
}


#define	TOP_PRIORITY 4
int XWindow::ParseExpressionPriority(core::XLexer& lex, int priority,
	XWinVar* var, int component)
{
	core::XLexToken token;
	int a;

	if (priority == 0) {
		return ParseTerm(lex, var, component);
	}


	a = ParseExpressionPriority(lex, priority - 1, var, component);

	if (!lex.ReadToken(token)) {
		// we won't get EOF in a real file, but we can
		// when parsing from generated strings
		return a;
	}


	// assume that anything else terminates the expression
	// not too robust error checking...
	lex.UnreadToken(token);

	return a;
}


int XWindow::ParseExpression(core::XLexer& lex, XWinVar* var)
{
	return ParseExpressionPriority(lex, TOP_PRIORITY, var);
}


bool XWindow::ParseString(core::XLexer& lex, core::string& out)
{
	core::XLexToken tok;
	if (lex.ReadToken(tok)) {
		out.assign(tok.begin(), tok.end());
		return true;
	}
	return false;
}

bool XWindow::ParseVar(const core::XLexToken& token, core::XLexer& lex)
{
	const char* nameBegin = token.begin();
	const char* nameEnd = token.end();

	using namespace core::strUtil;

	if (IsEqualCaseInsen(nameBegin, nameEnd, "name"))
	{
		// string
		core::XLexToken tok;
		if (lex.ReadToken(tok)) 
		{
			if (tok.length() > GUI_MAX_WINDOW_NAME_LEN)
			{
				core::StackString<256> temp(tok.begin(), tok.end());
				X_ERROR("Gui", "'name' var value exceeds length limit of: %i '%s' -> %i",
					GUI_MAX_WINDOW_NAME_LEN, temp.c_str(), tok.length());
				return false;
			}
		
			name_.set(tok.begin(), tok.end());
		}
		else
		{
			X_WARNING("Gui", "failed to parse 'name' var for menu");
			// return false here?
		}
	}
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "bordersize"))
	{
		borderSize_ = lex.ParseFloat();
	}
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "textAlignX"))
	{
		textAlignX_ = lex.ParseFloat();
	}
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "textAlignY"))
	{
		textAlignY_ = lex.ParseFloat();
	}
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "textAlign"))
	{
		// we will get these as numbers.
		int value = lex.ParseInt();

		if (value >= 0 && value <= TextAlign::MAX_VALUE)
			textAlign_ = (TextAlign::Enum)value;
		else
		{
			X_WARNING("Gui", "invalud value for textAlign: %i valid values(%i-%i)", 
				value, 0, TextAlign::MAX_VALUE);
		}
	}
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "shadowText"))
	{
		shadowText_ = lex.ParseBool();
	}
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "noCursor"))
	{
		if (lex.ParseBool())
			flags_.Set(WindowFlag::NO_CURSOR);
	}
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "noClip"))
	{
		if (lex.ParseBool())
			flags_.Set(WindowFlag::NO_CLIP);
	}
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "modal"))
	{
		if (lex.ParseBool())
			flags_.Set(WindowFlag::FULLSCREEN);
	}
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "fullscreen"))
	{
		if (lex.ParseBool())
			flags_.Set(WindowFlag::FULLSCREEN);
		else
			flags_.Remove(WindowFlag::FULLSCREEN); 
	}
	else
	{
		// you silly fish, get back on my dish, it is my only wish.
		// hiss hiss. i'm done, you can move on now..
		return false;
	}

	// we only get here if we found one
	return true;
}

bool XWindow::ParseRegEntry(const core::XLexToken& token, core::XLexer& lex)
{
	core::StackString512 name(token.begin(), token.end());
	core::XLexToken token2;
	int i;

	// find this register?
	XWinVar* pVar = GetWinVarByName(name.c_str());
	if (pVar)
	{
		// find it.
		for (i = 0; i < s_NumRegisterVars; i++) {
			if (name.isEqual(s_RegisterVars[i].name)) {
				regList_.AddReg(name.c_str(), s_RegisterVars[i].type, lex, this, pVar);
				return true;
			}
		}

		return true;
	}

	if (lex.ReadToken(token2))
	{

	}

	return false;
}


bool XWindow::ParseScriptFunction(const core::XLexToken& token, core::XLexer& lex)
{
	for (int i = 0; i < ScriptFunction::ENUM_COUNT; i++)
	{
		if (core::strUtil::IsEqualCaseInsen(token.begin(), token.end(), s_ScriptNames[i]))
		{
			return ParseScript(lex);
		//	scripts[i] = new idGuiScriptList;
		//	return ParseScript(src, *scripts[i]);
		}
	}

	return false;
}


bool XWindow::ParseScript(core::XLexer& lex)
{
	core::XLexToken token;
	int nest = 0;

	while (1)
	{
		if (!lex.ReadToken(token)) {
			return false;
		}

		if (token.isEqual("{")) {
			nest++;
		}

		if (token.isEqual("}")) {
			if (nest-- <= 0) {
				return true;
			}
		}
	}

	return false;
}


XWinVar* XWindow::GetWinVarByName(const char* name)
{
	X_ASSERT_NOT_NULL(name);
	XWinVar* retVar = nullptr;

	const char* nameBegin = name;
	const char* nameEnd = name + core::strUtil::strlen(name);

	using namespace core::strUtil;

	if (IsEqualCaseInsen(nameBegin, nameEnd, "rect"))
	{
		retVar = &rect_;
	}
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "backColor"))
	{
		retVar = &backColor_;
	}
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "foreColor"))
	{
		retVar = &foreColor_;
	}
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "hoverColor"))
	{
		retVar = &hoverColor_;
	}
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "borderColor"))
	{
		retVar = &borderColor_;
	}
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "visable"))
	{
		retVar = &visable_;
	}
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "hideCursor"))
	{
		retVar = &hideCursor_;
	}
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "textScale"))
	{
		retVar = &textScale_;
	}
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "text"))
	{
		retVar = &text_;
	}

	return retVar;
}
// -------------- Overrides ---------------


void XWindow::draw(int time, float x, float y)
{

}

void XWindow::activate(bool activate)
{

}

void XWindow::gainFocus(void)
{

}

void XWindow::loseFocus(void)
{

}

void XWindow::gainCapture(void)
{

}

void XWindow::loseCapture(void)
{

}

void XWindow::sized(void)
{

}

void XWindow::moved(void)
{

}

void XWindow::mouseExit(void)
{

}

void XWindow::mouseEnter(void)
{

}



X_NAMESPACE_END
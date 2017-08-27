#include "stdafx.h"
#include "XWindow.h"


#include <IFont.h>
#include <IRender.h>
#include <IFileSys.h>
#include <IPrimativeContext.h>
#include <IFont.h>

#include "GuiManger.h"
#include "Material\MaterialManager.h"


X_NAMESPACE_BEGIN(engine)

namespace gui
{


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
	{ "background", RegisterType::STRING },

};


const char* XWindow::s_ScriptNames[XWindow::ScriptFunction::ENUM_COUNT] = {
	"onMouseEnter",
	"onMouseExit",
	"onEsc",
	"onEnter",
	"onOpen",
	"onClose",
	"action"
};


const int XWindow::s_NumRegisterVars = sizeof(s_RegisterVars) / sizeof(XRegEntry);

bool XWindow::s_registerIsTemporary[MAX_EXPRESSION_REGISTERS]; 


XWindow::XWindow(XGui* pGui) :
	children_(g_3dEngineArena),
	drawWindows_(g_3dEngineArena),
	timeLineEvents_(g_3dEngineArena),
	transitions_(g_3dEngineArena),
	ops_(g_3dEngineArena),
	expressionRegisters_(g_3dEngineArena),
	init_(false),
	pGui_(pGui)
{
	X_ASSERT_NOT_NULL(pGui);
	init();
}

XWindow::~XWindow()
{
	clear();
}

void XWindow::init(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pFontSys);

	if (init_) {
		return;
	}

	init_ = true;

	// reg's
	backColor_ = g_DefaultColor_Back;
	foreColor_ = g_DefaultColor_Fore;
	hoverColor_ = g_DefaultColor_Hover;
	borderColor_ = g_DefaultColor_Border;
	visable_ = true;
	hideCursor_ = false;
	textScale_ = g_DefaultTextScale;
	// ~

	// default styles
	style_ = WindowStyle::FILLED;
	borderStyle_ = WindowBorderStyle::FULL;


	borderSize_ = 0.f; // no border by default.
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
	pFont_ = gEnv->pFontSys->GetDefault();
	pBackgroundMat_ = nullptr;

	hover_ = false;

	core::zero_object(scripts_);
}

void XWindow::clear(void)
{
	if (!init_)
		return;

	regList_.Reset();

	// we only clear instead of free.
	// saves reallocating again for reload.
	// memory is free when object is deleted.
	children_.clear();
	timeLineEvents_.clear();
	expressionRegisters_.clear();

	// delete any scripts.
	for (int i = 0; i < ScriptFunction::ENUM_COUNT; i++) {
		if (scripts_[i]) {
			X_DELETE_AND_NULL(scripts_[i], g_3dEngineArena);
		}
	}

	// release mat.
	if (pBackgroundMat_) {
		gEngEnv.pMaterialMan_->releaseMaterial(pBackgroundMat_);
	}

	init_ = false;
}

void XWindow::reset(void)
{
	clear();
	init();
}



// -------------- Parsing ---------------
void XWindow::SaveExpressionParseState()
{
	pSaveTemps_ = X_NEW_ARRAY(bool, MAX_EXPRESSION_REGISTERS, g_3dEngineArena, "ParseState");
	memcpy(pSaveTemps_, s_registerIsTemporary, MAX_EXPRESSION_REGISTERS * sizeof(bool));
}

void XWindow::RestoreExpressionParseState()
{
	memcpy(s_registerIsTemporary, pSaveTemps_, MAX_EXPRESSION_REGISTERS * sizeof(bool));
	X_DELETE_ARRAY(pSaveTemps_,g_3dEngineArena);
}


bool XWindow::Parse(core::XParser& lex)
{
	// clear it.
	reset();

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
			XWindow* win = X_NEW(XWindow,g_3dEngineArena,"ItemDef")(pGui_);
			win->setParent(this);

			SaveExpressionParseState();
				win->Parse(lex);
			RestoreExpressionParseState();

			addChild(win);
		//	SetFocus(win, false);

		}
		else if (token.isEqual("onTime"))
		{
			// timed event.
			XTimeLineEvent* event = X_NEW(XTimeLineEvent, g_3dEngineArena, "GuiTimeEvent");

			if (!lex.ReadToken(token)) {
				return false;
			}
			if (token.length() > 64) {
				X_ERROR("Gui", "time value is too long. line(%i)", token.GetLine());
				return false;
			}
			if (token.GetType() != core::TokenType::NUMBER) {
				X_ERROR("Gui", "expected number after 'onTime' line(%i)", token.GetLine());
				return false;
			}

			core::StackString<64> temp(token.begin(), token.end());

			event->time.SetMilliSeconds(core::strUtil::StringToFloat<float>(temp.c_str()) );

			if (!ParseScript(lex, *event->script))
			{
				X_ERROR("Gui", "error parsing 'onTime' on line(%i)", token.GetLine());
				return false;
			}

			timeLineEvents_.append(event);
		}
		else if (ParseScriptFunction(token, lex))
		{
			// it's a function definition.
		}
		else if (ParseVar(token, lex))
		{
			// it's a var :)
		}
		else if (ParseRegEntry(token, lex))
		{
			// it's a registry var.
		}



		if (!lex.ReadToken(token))
		{
			X_ERROR("Gui", "error while parsing windowDef");
			return false;
		}
	}

	EvalRegs(-1, true);

	SetupFromState();

	return true;
}


bool XWindow::Parse(core::XFile* pFile)
{
	X_ASSERT_NOT_NULL(pFile);

	// we read the info for this window out.
	// read in the var's one by one.
	// this will be a memory file so it's cheap.
	reset();

	// these are all vars and need to be parsed diffrent
	// depending on the type.
	rect_.fromFile(pFile);
	backColor_.fromFile(pFile);
	foreColor_.fromFile(pFile);
	hoverColor_.fromFile(pFile);
	borderColor_.fromFile(pFile);
	visable_.fromFile(pFile);
	hideCursor_.fromFile(pFile);
	textScale_.fromFile(pFile);
	// two srings.
	text_.fromFile(pFile);
	background_.fromFile(pFile);


	// style etc
	pFile->readObj(style_);
	// border
	pFile->readObj(borderStyle_);
	pFile->readObj(borderSize_);
	// tezt align
	pFile->readObj(textAlignX_);
	pFile->readObj(textAlignY_);
	pFile->readObj(textAlign_);
	// shadow
	pFile->readObj(shadowText_);
	// flags
	pFile->readObj(flags_);

	// read name :|
	pFile->readObj(name_); // it's Obj since stack string.

	// i still need to read the GuiScripts.
	// and stuff like timeline events / transistions
	// o.o

	// numchildren at the end.
	size_t i, numChildren;
	pFile->readObj(numChildren);

	if(numChildren > 0)
	{
		if (numChildren < GUI_MENU_MAX_ITEMS)
		{
			for (i = 0; i < numChildren; i++)
			{
				XWindow* pWin = X_NEW(XWindow, g_3dEngineArena, "ItemDef")(pGui_);
				pWin->setParent(this);
				pWin->Parse(pFile);

				addChild(pWin);
			}
		}
		else
		{
			X_ERROR("Gui", "excceded max child items num: %i, max: %i", 
				numChildren, GUI_MENU_MAX_ITEMS);
		}
	}

	SetupFromState();
	return true;
}

bool XWindow::WriteToFile(core::XFile* pFile)
{
	// for this i will need towrite all the vars to a 
	// file and the scripts as well as all the transitions etc.	
	// guess it's time to write the overrides for hte vars to be able tosae to a file.
	rect_.toFile(pFile);
	backColor_.toFile(pFile);
	foreColor_.toFile(pFile);
	hoverColor_.toFile(pFile);
	borderColor_.toFile(pFile);
	visable_.toFile(pFile);
	hideCursor_.toFile(pFile);
	textScale_.toFile(pFile);
	// two srings.
	text_.toFile(pFile);
	background_.toFile(pFile);


	// style etc
	pFile->writeObj(style_);
	// border
	pFile->writeObj(borderStyle_);
	pFile->writeObj(borderSize_);
	// tezt align
	pFile->writeObj(textAlignX_);
	pFile->writeObj(textAlignY_);
	pFile->writeObj(textAlign_);
	// shadow
	pFile->writeObj(shadowText_);
	// flags
	pFile->writeObj(flags_);

	// read name :|
	pFile->writeObj(name_);


	// need to write gui scripts and other things here.

	


	// write how many children we have.
	uint32_t numChildren = safe_static_cast<uint32_t, size_t>(children_.size());
	pFile->writeObj(numChildren);

	Childit it = children_.begin();
	for(; it != children_.end(); ++it)
	{
		(*it)->WriteToFile(pFile);
	}
	return true;
}

void XWindow::SetupFromState(void) 
{
	if (borderSize_ > 0.f) {
		flags_.Set(WindowFlag::BORDER);
	}

	if (style_ == WindowStyle::SHADER) {
		pBackgroundMat_ = gEngEnv.pMaterialMan_->loadMaterial(background_.c_str());
	
#if 0
		engine::MaterialCat::Enum cat = pBackgroundMat_->getType();
		if (type != engine::MaterialCat::UI) {
			X_WARNING("Gui", "Material %s is not a GUI material", background_.c_str());
			pBackgroundMat_ = pMaterialManager_->getDefaultMaterial();
		}
#endif
	}
}

void XWindow::FixUpParms(void)
{
	size_t i, num;

	num = children_.size();
	for (i = 0; i < num; i++) {
		children_[i]->FixUpParms();
	}

	for (i = 0; i < ScriptFunction::ENUM_COUNT; i++) {
		if (scripts_[i]) {
			scripts_[i]->FixUpParms(this);
		}
	}

	
	num = timeLineEvents_.size();
	for (i = 0; i < num; i++) {
		timeLineEvents_[i]->script->FixUpParms(this);
	}
	/*
	c = namedEvents.Num();
	for (i = 0; i < c; i++) {
		namedEvents[i]->mEvent->FixupParms(this);
	}
	*/

	if (flags_.IsSet(WindowFlag::DESKTOP)) {
		calcClientRect();
	}
}


void XWindow::StartTransition()
{
	flags_.Set(WindowFlag::IN_TRANSITION);
}

void XWindow::AddTransition(XWinVar* dest, Vec4f from, Vec4f to,
	int timeMs, float accelTime, float decelTime)
{
	int startTime = static_cast<int>(lastTimeRun_.GetMilliSeconds());

	XTransitionData data;
	data.pData = dest;
	data.interp.Init(startTime, (int)(accelTime * timeMs), (int)(decelTime * timeMs), timeMs, from, to);
	transitions_.append(data);
}

void XWindow::ResetTime(int timeMs)
{
	X_ASSERT_NOT_IMPLEMENTED();
	core::TimeVal time(0ll); //  pTimer_->GetFrameStartTime(core::ITimer::Timer::UI);
	core::TimeVal temp;

	int UItimeMs = static_cast<int>(time.GetMilliSeconds());

	temp.SetMilliSeconds(timeMs);

	timeLine_.SetMilliSeconds(UItimeMs - timeMs);

	size_t i, num = timeLineEvents_.size();
	for (i = 0; i < num; i++) {
		if (timeLineEvents_[i]->time >= temp) {
			timeLineEvents_[i]->pending = true;
		}
	}

	// noTime = false;

	num = transitions_.size();
	for (i = 0; i < num; i++) {
		XTransitionData* data = &transitions_[i];
		if (data->interp.IsDone(UItimeMs) && data->pData) {
			transitions_.removeIndex(i);
			i--;
			num--;
		}
	}

}

bool XWindow::RunTimeEvents(core::TimeVal time)
{
	if (lastTimeRun_ == time)
		return false;

	lastTimeRun_ = time;

	if (flags_.IsSet(WindowFlag::IN_TRANSITION)) {
		Transition();
	}

	Time(time);

	size_t i, c = children_.size();
	for (i = 0; i < c; i++) {
		children_[i]->RunTimeEvents(time);
	}

	return true;
}

void XWindow::Time(core::TimeVal time)
{
	if (timeLine_.GetValue() == 0) {
		timeLine_ = time;
	}

	core::Array<XTimeLineEvent*>::Iterator it;
	core::TimeVal relTime = time - timeLine_;

	for (it = timeLineEvents_.begin(); it != timeLineEvents_.end(); ++it)
	{
		XTimeLineEvent& event = *(*it);

		if (event.pending)
		{
			if (relTime >= event.time)
			{
				event.pending = false;
				RunScriptList(event.script);
			}
		}
	}

}

void XWindow::Transition(void)
{
	size_t i, num = transitions_.size();
	bool clear = true;

	int timeMs = static_cast<int>(lastTimeRun_.GetMilliSeconds());

	for (i = 0; i < num; i++) 
	{
		XTransitionData& data = transitions_[i];

		if (!data.pData)
		{
			X_ERROR("Gui", "transition data missing target var");
			continue;
		}

		VarType::Enum type = data.pData->getType();

		if (type == VarType::RECT)
		{
			XWinRect* r = static_cast<XWinRect*>(data.pData);

			if (data.interp.IsDone(timeMs))
			{
				r->Set(data.interp.GetEndValue());
			}
			else
			{
				clear = false;
				r->Set(data.interp.GetCurrentValue(timeMs));
			}
		}
		else if (type == VarType::COLOR)
		{
			XWinColor* color = static_cast<XWinColor*>(data.pData);

			if (data.interp.IsDone(timeMs))
			{
				color->Set(data.interp.GetEndValue());
			}
			else
			{
				clear = false;
				color->Set(data.interp.GetCurrentValue(timeMs));
			}

		}
	}

	if (clear) {
		transitions_.clear();
		flags_.Remove(WindowFlag::IN_TRANSITION);
	}
}


bool XWindow::RunScriptList(XGuiScriptList* src)
{
	if (src)
	{
		src->Execute(this);
		return true;
	}
	return false;
}

bool XWindow::RunScript(ScriptFunction::Enum func)
{
	if (scripts_[func]) // this if is kinda un-needed :}
		return RunScriptList(scripts_[func]);
	return false;
}


void XWindow::EvaluateRegisters(float* registers) 
{
	size_t i, erc, oc;
	int32_t	b;
	Vec4f v;
	xOpt* op;

	erc = expressionRegisters_.size();
	oc = ops_.size();


	// copy the constants
	for (i = WEXP_REG_NUM_PREDEFINED; i < erc; i++) {
		registers[i] = expressionRegisters_[i];
	}

	// copy the local and global parameters
	// registers[WEXP_REG_TIME] = 0; // gui->GetTime();


	for (i = 0; i < oc; i++) 
	{
		op = &ops_[i];
		if (op->b == -2) {
			continue;
		}
		switch (op->opType)
		{
			case OpType::ADD:
				registers[op->c] = registers[op->a] + registers[op->b];
				break;
			case OpType::SUBTRACT:
				registers[op->c] = registers[op->a] - registers[op->b];
				break;
			case OpType::MULTIPLY:
				registers[op->c] = registers[op->a] * registers[op->b];
				break;
			case OpType::DIVIDE:
				if (registers[op->b] == 0.0f)
				{
					X_WARNING("Gui", "Divide by zero in: %s", getName());
					registers[op->c] = registers[op->a];
				}
				else {
					registers[op->c] = registers[op->a] / registers[op->b];
				}
				break;
			case OpType::MOD:
				b = static_cast<int>(registers[op->b]);
				b = b != 0 ? b : 1;
				registers[op->c] = static_cast<float>(
					static_cast<int>(registers[op->a]) % b);
				break;

			
			case OpType::GT:
				registers[op->c] = registers[op->a] > registers[op->b];
				break;
			case OpType::GE:
				registers[op->c] = registers[op->a] >= registers[op->b];
				break;
			case OpType::LT:
				registers[op->c] = registers[op->a] < registers[op->b];
				break;
			case OpType::LE:
				registers[op->c] = registers[op->a] <= registers[op->b];
				break;
			case OpType::EQ:
				registers[op->c] = registers[op->a] == registers[op->b];
				break;
			case OpType::NE:
				registers[op->c] = registers[op->a] != registers[op->b];
				break;
			case OpType::COND:
				registers[op->c] = (registers[op->a]) ? registers[op->b] : registers[op->d];
				break;
			case OpType::AND:
				registers[op->c] = registers[op->a] && registers[op->b];
				break;
			case OpType::OR:
				registers[op->c] = registers[op->a] || registers[op->b];
				break;
			case OpType::VAR:
				// 'type cast': conversion from 'int' to 'Potato::gui::XWinVec4 *' of greater size

				X_DISABLE_WARNING(4312) 
				X_ASSERT_NOT_IMPLEMENTED();

				if (!op->a) {
					registers[op->c] = 0.0f;
					break;
				}
				if (op->b >= 0 && registers[op->b] >= 0 && registers[op->b] < 4) 
				{
					// grabs vector components
					XWinVec4 *var = reinterpret_cast<XWinVec4*>(op->a);
					registers[op->c] = ((Vec4f&)var)[static_cast<int>(registers[op->b])];
				}
				else {
					registers[op->c] = 0; // ((XWinVar*)(op->a))->x();
				}
				break;
			case OpType::VAR_STR:
				X_ASSERT_NOT_IMPLEMENTED();

				if (op->a) {
					XWinStr* var = reinterpret_cast<XWinStr*>(op->a);
					registers[op->c] = core::strUtil::StringToFloat<float>(var->c_str());
				}
				else {
					registers[op->c] = 0;
				}
				break;
			case OpType::VAR_FLOAT:
				X_ASSERT_NOT_IMPLEMENTED();

				if (op->a) {
					XWinFloat* var = reinterpret_cast<XWinFloat*>(op->a);
					registers[op->c] = *var;
				}
				else {
					registers[op->c] = 0;
				}
				break;
			case OpType::VAR_INT:
				X_ASSERT_NOT_IMPLEMENTED();

				if (op->a) {
					XWinInt* var = reinterpret_cast<XWinInt*>(op->a);
					registers[op->c] = static_cast<float>(*var);
				}
				else {
					registers[op->c] = 0;
				}
				break;
			case OpType::VAR_BOOL:
				X_ASSERT_NOT_IMPLEMENTED();

				if (op->a) {
					XWinBool* var = reinterpret_cast<XWinBool*>(op->a);
					registers[op->c] = *var;
				}
				else {
					registers[op->c] = 0;
				}
				break;
				X_ENABLE_WARNING(4312)
#if X_DEBUG
			default:
				X_ASSERT_UNREACHABLE();
				break;
#else
				X_NO_SWITCH_DEFAULT;
#endif // !X_DEBUG
				
		}
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

	for (i = WEXP_REG_NUM_PREDEFINED; 
		i < safe_static_cast<int, size_t>(expressionRegisters_.size()); i++)
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

int XWindow::ParseTerm(core::XParser& lex, XWinVar* var, int component)
{
	core::XLexToken token;
	int	a;

	X_UNUSED(var);
	X_UNUSED(component);

	lex.ReadToken(token);

	if (token.isEqual("("))
	{
		a = ParseExpression(lex);
		lex.ExpectTokenString(")");
		return a;
	}
	

	if (token.isEqual("time")) {
		return WEXP_REG_TIME;
	}

	// parse negative numbers
	if (token.isEqual("-")) 
	{
		lex.ReadToken(token);
		if (token.GetType() == core::TokenType::NUMBER || token.isEqual("."))
		{
			return ExpressionConstant(-(float)token.GetFloatValue());
		}
		return 0;
	}

	if (token.GetType() == core::TokenType::NUMBER || token.isEqual(".") || token.isEqual("-")) {
		return ExpressionConstant((float)token.GetFloatValue());
	}

	return 0;
}


int XWindow::ExpressionTemporary(void)
{
	if (expressionRegisters_.size() == MAX_EXPRESSION_REGISTERS) {
		X_WARNING("Gui", "%s reached max expression registers. max: %i",
			this->getName(), MAX_EXPRESSION_REGISTERS);
		return 0;
	}

	int i = safe_static_cast<int,size_t>(expressionRegisters_.size());
	s_registerIsTemporary[i] = true;
	i = safe_static_cast<int,size_t>(expressionRegisters_.append(0));
	return i;
}


xOpt* XWindow::ExpressionOp(void)
{
	if (ops_.size() == MAX_EXPRESSION_OPS) {
		X_WARNING("Gui", "%s reached max expression ops. max: %i",
			this->getName(), MAX_EXPRESSION_OPS);
		return &ops_[0];
	}

	xOpt wop;

	size_t i = ops_.append(wop);
	return &ops_[i];
}

int XWindow::EmitOp(int a, int b, OpType::Enum opType, xOpt **opp)
{
	xOpt* op;

	op = ExpressionOp();

	op->opType = opType;
	op->a = a;
	op->b = b;
	op->c = ExpressionTemporary();

	if (opp) {
		*opp = op;
	}
	return op->c;
}

int XWindow::ParseEmitOp(core::XParser& lex, int a, OpType::Enum opType,
	int priority, xOpt** opp)
{
	int b = ParseExpressionPriority(lex , priority);
	return EmitOp(a, b, opType, opp);
}

#define	TOP_PRIORITY 4
int XWindow::ParseExpressionPriority(core::XParser& lex, int priority,
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


	if (priority == 1 && token.isEqual("*")) {
		return ParseEmitOp(lex, a, OpType::MULTIPLY, priority);
	}
	if (priority == 1 && token.isEqual("/")) {
		return ParseEmitOp(lex, a, OpType::DIVIDE, priority);
	}
	if (priority == 1 && token.isEqual("%")) {	// implied truncate both to integer
		return ParseEmitOp(lex, a, OpType::MOD, priority);
	}
	if (priority == 2 && token.isEqual("+")) {
		return ParseEmitOp(lex, a, OpType::ADD, priority);
	}
	if (priority == 2 && token.isEqual("-")) {
		return ParseEmitOp(lex, a, OpType::SUBTRACT, priority);
	}
	if (priority == 3 && token.isEqual(">")) {
		return ParseEmitOp(lex, a, OpType::GT, priority);
	}
	if (priority == 3 && token.isEqual(">=")) {
		return ParseEmitOp(lex, a, OpType::GE, priority);
	}
	if (priority == 3 && token.isEqual("<")) {
		return ParseEmitOp(lex, a, OpType::LT, priority);
	}
	if (priority == 3 && token.isEqual("<=")) {
		return ParseEmitOp(lex, a, OpType::LE, priority);
	}
	if (priority == 3 && token.isEqual("==")) {
		return ParseEmitOp(lex, a, OpType::EQ, priority);
	}
	if (priority == 3 && token.isEqual("!=")) {
		return ParseEmitOp(lex, a, OpType::NE, priority);
	}
	if (priority == 4 && token.isEqual("&&")) {
		return ParseEmitOp(lex, a, OpType::AND, priority);
	}
	if (priority == 4 && token.isEqual("||")) {
		return ParseEmitOp(lex, a, OpType::OR, priority);
	}
	if (priority == 4 && token.isEqual("?")) {
		xOpt* oop = nullptr;
		int o = ParseEmitOp(lex, a, OpType::COND, priority, &oop);
		if (!lex.ReadToken(token)) {
			return o;
		}
		if (token.isEqual(":")) {
			a = ParseExpressionPriority(lex, priority - 1, var);
			oop->d = a;
		}
		return o;
	}

	// assume that anything else terminates the expression
	// not too robust error checking...
	lex.UnreadToken(token);

	return a;
}


int XWindow::ParseExpression(core::XParser& lex, XWinVar* var)
{
	return ParseExpressionPriority(lex, TOP_PRIORITY, var);
}


bool XWindow::ParseString(core::XParser& lex, core::string& out)
{
	core::XLexToken tok;
	if (lex.ReadToken(tok)) {
		out.assign(tok.begin(), tok.end());
		return true;
	}
	return false;
}

bool XWindow::ParseVar(const core::XLexToken& token, core::XParser& lex)
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
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "style"))
	{
		int style32 = lex.ParseInt();

		if (style32 >= 0 && style32 < WindowStyle::ENUM_COUNT)
			style_ = (WindowStyle::Enum)style32;
		else
		{
			X_WARNING("Gui", "unknown style: %i", style32);
		}
	}
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "borderstyle"))
	{
		int style32 = lex.ParseInt();

		if (style32 >= 0 && style32 < WindowBorderStyle::ENUM_COUNT)
			borderStyle_ = (WindowBorderStyle::Enum)style32;
		else
		{
			X_WARNING("Gui", "unknown border stlye: %i", style32);
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

bool XWindow::ParseRegEntry(const core::XLexToken& token, core::XParser& lex)
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


bool XWindow::ParseScriptFunction(const core::XLexToken& token, core::XParser& lex)
{
	for (int i = 0; i < ScriptFunction::ENUM_COUNT; i++)
	{
		if (core::strUtil::IsEqualCaseInsen(token.begin(), token.end(), s_ScriptNames[i]))
		{
			scripts_[i] = X_NEW(XGuiScriptList,g_3dEngineArena,"guiScriptList");
			return ParseScript(lex, *scripts_[i]);
		}
	}

	return false;
}


bool XWindow::ParseScript(core::XParser& lex, XGuiScriptList& list )
{
	core::XLexToken token;
	int nest = 0;

	if (!lex.ExpectTokenString("{")) {
		return false;
	}

	X_DISABLE_WARNING(4127)
	while (true)
	X_ENABLE_WARNING(4127)
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

		lex.UnreadToken(token);

		if (token.isEqual("{")) {
			return false;
		}

		XGuiScript* gs = X_NEW(XGuiScript,g_3dEngineArena,"GuiScript");

		gs->Parse(lex);
		list.append(gs);
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
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "visible"))
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
	else if (IsEqualCaseInsen(nameBegin, nameEnd, "background"))
	{
		retVar = &background_;
	}

	return retVar;
}



// ----------------------------------------

// Drawing
void XWindow::reDraw(engine::IPrimativeContext* pDrawCon)
{
	X_PROFILE_BEGIN("GuiDraw", core::profiler::SubSys::ENGINE3D);
	X_ASSERT_NOT_IMPLEMENTED();

	core::TimeVal time = core::TimeVal(0ll); // pTimer_->GetFrameStartTime(core::ITimer::Timer::UI);

	if (flags_.IsSet(WindowFlag::DESKTOP)) {
		RunTimeEvents(time);
	}

	if (!visable_) {
		return;
	}

	calcClientRect();

	if (style_ == WindowStyle::DVAR_SHADER) {
		// this should not be null even if load fails.
		X_ASSERT_NOT_NULL(pBackgroundMat_);
	
		// check if the dvar has changed.
		if (!core::strUtil::IsEqual(pBackgroundMat_->getName(),background_.getName()))
		{
			gEngEnv.pMaterialMan_->releaseMaterial(pBackgroundMat_);
			// this might cause a material to be loaded from disk.
			pBackgroundMat_ = gEngEnv.pMaterialMan_->loadMaterial(background_.c_str());
		}
	}

	if (style_ != WindowStyle::EMPTY)
	{
		drawBackground(pDrawCon, rectDraw_);
		drawBorder(pDrawCon, rectDraw_);
	}

	draw(pDrawCon, time, rectClient_.x1, rectClient_.y1);

	if (gEngEnv.pGuiMan_->ShowDeubug()) {
		drawDebug(pDrawCon);
	}

	Childit it = children_.begin();
	for (; it != children_.end(); ++it)
	{
		(*it)->reDraw(pDrawCon);
	}

	// now draw cursor
	if (flags_.IsSet(WindowFlag::DESKTOP)) 
	{
		if (!flags_.IsSet(WindowFlag::MOVEABLE) && !hideCursor_)
		{
			pGui_->DrawCursor(pDrawCon);
		}
	}
}

void XWindow::drawDebug(engine::IPrimativeContext* pDrawCon)
{
	core::StackString<2048> str;

	str.appendFmt("Name: '%s'\n", name_.c_str());
	str.appendFmt("Text: '%s'\n", text_.c_str());
	str.appendFmt("Draw: %g %g %g %g\n", rectDraw_.x1, rectDraw_.y1, rectDraw_.x2, rectDraw_.y2);
	str.appendFmt("Client: %g %g %g %g\n", rectClient_.x1, rectClient_.y1, rectClient_.x2, rectClient_.y2);
	str.appendFmt("Backcolor: %g %g %g %g\n", backColor_.r(), backColor_.g(),
		backColor_.b(), backColor_.a());

	// make the text visable.
	float v = (backColor_.r() + backColor_.g() + backColor_.b()) / 3.f > 0.5f ? 0.f : 1.f;


	// pos :Z?
	Vec3f pos;
	pos = Vec3f(rectClient_.getUpperLeft());
	pos.x += 3.f;
	pos.y += 3.f;

	font::TextDrawContext ctx;
	ctx.col = Color(v, v, v);

	pDrawCon->drawText(pos, ctx, str.begin(), str.end());
}



void XWindow::drawBorder(engine::IPrimativeContext* pDrawCon, const Rectf& drawRect)
{
	if (flags_.IsSet(WindowFlag::BORDER))
	{
		if (borderColor_.a() > 0.f)
		{

			switch (borderStyle_)
			{
				case WindowBorderStyle::FULL:
					pDrawCon->drawRectSS(drawRect, borderColor_);
					break;
				case WindowBorderStyle::HORZ:
					pDrawCon->drawLineSS(drawRect.getUpperLeft(), borderColor_,
						drawRect.getUpperRight(), borderColor_);
					pDrawCon->drawLineSS(drawRect.getLowerLeft(), borderColor_,
						drawRect.getLowerRight(), borderColor_);
					break;
				case WindowBorderStyle::VERT:
					pDrawCon->drawLineSS(drawRect.getUpperLeft(), borderColor_,
						drawRect.getLowerLeft(), borderColor_);
					pDrawCon->drawLineSS(drawRect.getUpperRight(), borderColor_,
						drawRect.getLowerRight(), borderColor_);
					break;
				case WindowBorderStyle::GRADIENT:
					// humm
					X_ASSERT_NOT_IMPLEMENTED();

					break;
				case WindowBorderStyle::RAISED:
				{
					const Color LTColor = borderColor_;
					Color BRColor = borderColor_;

					BRColor.shade(-30.f);

					// top
					pDrawCon->drawLineSS(drawRect.getUpperLeft(), LTColor,
						drawRect.getUpperRight(), LTColor);
					// bottom
					pDrawCon->drawLineSS(drawRect.getLowerLeft(), BRColor,
						drawRect.getLowerRight(), BRColor);
					// left
					pDrawCon->drawLineSS(drawRect.getUpperLeft(), LTColor,
						drawRect.getLowerLeft(), LTColor);
					// right
					pDrawCon->drawLineSS(drawRect.getUpperRight(), BRColor,
						drawRect.getLowerRight(), BRColor);
					
					break;
				}
				case WindowBorderStyle::SUNKEN:
				{
					Color LTColor = borderColor_;
					Color BRColor = borderColor_;

					LTColor.shade(-30.f);

					// top
					pDrawCon->drawLineSS(drawRect.getUpperLeft(), LTColor,
						drawRect.getUpperRight(), LTColor);
					// bottom
					pDrawCon->drawLineSS(drawRect.getLowerLeft(), BRColor,
						drawRect.getLowerRight(), BRColor);
					// left
					pDrawCon->drawLineSS(drawRect.getUpperLeft(), LTColor,
						drawRect.getLowerLeft(), LTColor);
					// right
					pDrawCon->drawLineSS(drawRect.getUpperRight(), BRColor,
						drawRect.getLowerRight(), BRColor);
					
					break;
				}
				case WindowBorderStyle::NONE:
					break;
#if X_DEBUG
				default:
					X_ASSERT_UNREACHABLE();
					break;
#else
				default:
					X_NO_SWITCH_DEFAULT;
#endif // !X_DEBUG
			}
		}
	}
}

void XWindow::calcClientRect(void)
{
	const Rectf& rect = rect_;

	const float width = static_cast<float>(gEnv->pRender->getDisplayRes().x);
	const float height = static_cast<float>(gEnv->pRender->getDisplayRes().y);

	// 800x600 virtual.
	const float scale_x = width / 800;
	const float scale_y = height / 600;

	rectClient_ = rect_;
	rectClient_.x1 *= scale_x;
	rectClient_.y1 *= scale_y;
	rectClient_.x2 *= scale_x;
	rectClient_.y2 *= scale_y;

	// make screen space baby.
	// convert to 0-2 first.
	rectDraw_.x1 = (rect.x1 / 400);
	rectDraw_.y1 = (rect.y1 / 300);
	rectDraw_.x2 = (rect.x2 / 400);
	rectDraw_.y2 = (rect.y2 / 300);

	rectDraw_.x1 *= scale_x;
	rectDraw_.y1 *= scale_y;
	rectDraw_.x2 *= scale_x;
	rectDraw_.y2 *= scale_y;

	if (rectClient_.getHeight() > 0.f && rectClient_.getWidth() > 0.f) {

		// set text rect to client
		rectText_ = rectClient_;
		// offset x,y
		// this will not move the bottom right of the rect
		rectText_.x1 += textAlignX_;
		rectText_.y1 += textAlignY_;
	}

}
// -------------- Overrides ---------------


void XWindow::draw(engine::IPrimativeContext* pDrawCon, core::TimeVal time, float x_, float y_)
{
	X_UNUSED(pDrawCon, time);

	// draw the text.
	if (text_.getLength() == 0) {
		return;
	}

	Rectf rect = rectClient_;

	float x = x_;
	float y = y_;

	font::TextDrawContext contex;
	contex.widthScale = 1.0f;
	contex.col = Colorf(foreColor_);

	if (!flags_.IsSet(WindowFlags::NO_CLIP))
	{
		contex.flags.Set(font::DrawTextFlag::CLIP);
		contex.clip = rect;
	}

	Vec2f textDimension = pFont_->GetTextSize(text_.begin(), text_.end(), contex);

	// top align
	switch (textAlign_)
	{
		case TextAlign::TOP_LEFT:
		case TextAlign::TOP_CENTER:
		case TextAlign::TOP_RIGHT:
//		case TextAlign::TOP:

			break;
		default:
			break;
	}

	// middle align
	switch (textAlign_)
	{
		case TextAlign::MIDDLE_LEFT:
		case TextAlign::MIDDLE_CENTER:
		case TextAlign::MIDDLE_RIGHT:
//		case TextAlign::MIDDLE:
			y += (rect.getHeight() / 2);
			y -= (textDimension.y / 2);
			break;
		default:
			break;
	}

	// bottom align
	switch (textAlign_)
	{
		case TextAlign::BOTTOM_LEFT:
		case TextAlign::BOTTOM_CENTER:
		case TextAlign::BOTTOM_RIGHT:
//		case TextAlign::BOTTOM:
			y += rect.getHeight();
			y -= textDimension.y;
			break;
		default:
			break;
	}

	// left align
	switch (textAlign_)
	{
		case TextAlign::TOP_LEFT:
		case TextAlign::MIDDLE_LEFT:
		case TextAlign::BOTTOM_LEFT:
		case TextAlign::LEFT:

			break;
		default:
			break;
	}

	// center
	switch (textAlign_)
	{
		case TextAlign::TOP_CENTER:
		case TextAlign::MIDDLE_CENTER:
		case TextAlign::BOTTOM_CENTER:
		case TextAlign::CENTER:
			x += (rect.getWidth() / 2);
			x -= (textDimension.x / 2);
			break;
		default:
			break;
	}

	// right
	switch (textAlign_)
	{
		case TextAlign::TOP_RIGHT:
		case TextAlign::MIDDLE_RIGHT:
		case TextAlign::BOTTOM_RIGHT:
		case TextAlign::RIGHT:
			x += rect.getWidth();
			x -= textDimension.x;
			break;
		default:
			break;
	}

	// add the offsets.
	x += textAlignX_;
	y += textAlignY_;

	// draw it.
	pDrawCon->drawText(Vec3f(x, y, 0.f), contex, text_.c_str());
}

void XWindow::drawBackground(engine::IPrimativeContext* pDrawCon, const Rectf& drawRect)
{
	if (style_ == WindowStyle::FILLED)
	{
		pDrawCon->drawQuadSS(drawRect, backColor_);
	}
	else if (style_ == WindowStyle::GRADIENT)
	{

	}
	else if (style_ == WindowStyle::SHADER || style_ == WindowStyle::DVAR_SHADER)
	{
#if 1
		X_ASSERT_NOT_IMPLEMENTED();
#else
		const Color& col = backColor_;

		pRender_->setGUIShader(true);

		render::shader::XShaderItem& item = pBackgroundMat_->getShaderItem();
		render::shader::XTextureResource* texRes = item.pResources_->getTexture(render::shader::ShaderTextureIdx::DIFFUSE);
		texture::TexID texId = texRes->pITex->getTexID();

		pRender_->DrawQuadImageSS(drawRect, texId, col);
#endif
	}
}


// input
bool XWindow::OnInputEvent(const input::InputEvent& event)
{
	using namespace input;

	if (flags_.IsSet(WindowFlag::DESKTOP))
	{
		// root window.

	}

	if (visable_)
	{
		if (event.deviceType == InputDeviceType::MOUSE)
		{
			Childit it = children_.begin();



			if (event.keyId == KeyId::MOUSE_LEFT)
			{
				for (; it != children_.end(); ++it)
				{


				}
			}
			else if (event.keyId == KeyId::MOUSE_RIGHT)
			{
				for (; it != children_.end(); ++it)
				{


				}
			}
			else if (event.keyId == KeyId::MOUSE_MIDDLE)
			{

			}
		}
	}
	return false;
}

bool XWindow::OnInputEventChar(const input::InputEvent& event)
{
	X_UNUSED(event);
	return false;
}


void XWindow::activate(bool activate)
{
	X_UNUSED(activate);
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

} // namespace gui

X_NAMESPACE_END
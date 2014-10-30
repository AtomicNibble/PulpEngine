#pragma once

#ifndef X_GUI_WINDOW_H_
#define X_GUI_WINDOW_H_

#include <IGui.h>

#include <String\Lexer.h>

#include "WinVar.h"

X_NAMESPACE_BEGIN(gui)

X_DECLARE_ENUM(RegisterType)
(
	BOOL,
	INT,
	FLOAT,
	VEC2,
	VEC3,
	VEC4,
	Color,
	Rect,
	STRING
);

// enum REGTYPE { VEC4 = 0, FLOAT, BOOL, INT, STRING, VEC2, VEC3, RECTANGLE, NUMTYPES };

class XWindowSimple;
class XWindow
{
public:
	typedef Flags<WindowFlag> WindowFlags;

	X_DECLARE_ENUM(ScriptFunction) (
		MOUSE_ENTER,
		MOUSE_LEAVE,
		ESC,
		ENTER
	);

	static const char* ScriptNames[ScriptFunction::ENUM_COUNT];

	static const char* RegisterVars[];
	static const int   NumRegisterVars;

public:
	XWindow();
	~XWindow();

	// Parent
	X_INLINE void setParent(XWindow* pParent);
	X_INLINE XWindow* getParent(void);

	// Flags
	X_INLINE void setFlag(WindowFlag::Enum flag);
	X_INLINE void clearFlags(void);
	X_INLINE WindowFlags getFlags(void) const;

	// Children
	X_INLINE void addChild(XWindow* pChild);
	X_INLINE void removeChild(XWindow* pChild);
	X_INLINE XWindow* getChild(int index);
	X_INLINE size_t	getNumChildren(void) const;
	X_INLINE uint32_t getIndexForChild(XWindow* pWindow) const;

	// Name
	X_INLINE const char* getName(void) const;

	// Drawing
	void reDraw(void);
	void drawDebug(void);
	void drawCaption(void);


	// Overrides
	virtual bool Parse(core::XLexer& lex);
	virtual void draw(int time, float x, float y);
	virtual void activate(bool activate);
	virtual void gainFocus(void);
	virtual void loseFocus(void);
	virtual void gainCapture(void);
	virtual void loseCapture(void);
	virtual void sized(void);
	virtual void moved(void);
	virtual void mouseExit(void);
	virtual void mouseEnter(void);

private:
	bool ParseString(core::XLexer& lex, core::string& out);
	bool ParseVar(const core::XLexToken& token, core::XLexer& lex);
	bool ParseRegEntry(const core::XLexToken& token, core::XLexer& lex);
	bool ParseScriptFunction(const core::XLexToken& token, core::XLexer& lex);
	bool ParseScript(core::XLexer& lex);

	XWinVar* GetWinVarByName(const char* name);

protected:
	Rectf rectDraw_;
	Rectf rectClient_;

	// Registers: shit that can be changed by code.
	XWinRect	rect_;
	XWinColor	backColor_;
	XWinColor	foreColor_;
	XWinColor	hoverColor_;
	XWinColor	borderColor_;
	XWinBool	visable_;
	XWinBool	hideCursor_;
	XWinFloat	textScale_;
	XWinStr		text_;
	// ~

	// variables, can only be set by the gui file.
	// can't change at runtime.
	float borderSize_;
	float textAlignX_;				// x offset from aligned position
	float textAlignY_;				// y offset from aligned position.
	TextAlign::Enum textAlign_;		// alignment type flags, LEFT, MIDDLE, BOTTOM, etc..
	bool shadowText_;				// 'shadow'
	bool __pad[2];
	WindowFlags flags_;
	core::StackString<GUI_MAX_WINDOW_NAME_LEN> name_;
	// ~

	uint32_t childId_; // if this is a child, this is it's id.

	XWindow* pParent_;
	XWindow* pFocusedChild_;	// if a child window has the focus
	XWindow* pCaptureChild_;	// if a child window has mouse capture
	XWindow* pOverChild_;		// if a child window has mouse capture

	font::IFFont* pFont_;

	core::Array<XWindow*> children_;

	bool	hover_;
	bool    ___pad[3];
};

#include "XWindow.inl"

X_NAMESPACE_END

#endif // !X_GUI_WINDOW_H_
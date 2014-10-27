#pragma once

#ifndef X_GUI_WINDOW_H_
#define X_GUI_WINDOW_H_

X_NAMESPACE_BEGIN(engine)


X_DECLARE_FLAGS(WindowFlag)
(
CAPTION,
CHILD,
BORDER,
SIZABLE,
MOVEABLE,
FOCUS,
SELECTED,
NOCURSOR,
ACTIVE
);


static const uint32_t GUI_CAPTION_HEIGHT = 16;
static const uint32_t GUI_SCROLLER_SIZE = 16;
static const uint32_t GUI_SCROLLBAR_SIZE = 16;

static const uint32_t GUI_MAX_WINDOW_NAME = 28;
static const uint32_t GUI_MAX_LIST_ITEMS = 1024;


class XWindow
{
public:
	typedef Flags<WindowFlag> WindowFlags;

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


protected:
	Rectf rect_;
	Rectf rectClient_;

	Color	backColor_;
	Color	foreColor_;
	Color	hoverColor_;
	Color	borderColor_;

	WindowFlags flags_;
	uint32_t childId_; // if this is a child, this is it's id.

	XWindow* pParent_;
	XWindow* pFocusedChild_;	// if a child window has the focus
	XWindow* pCaptureChild_;	// if a child window has mouse capture
	XWindow* pOverChild_;		// if a child window has mouse capture

	font::IFFont* pFont_;
	core::string text_;


	core::Array<XWindow*> children_;
	core::StackString<GUI_MAX_WINDOW_NAME> name_;

	bool	visible_;
	bool	hideCursor_;
	bool	hover_;
	bool    __pad[1];
};

#include "XWindow.inl"

X_NAMESPACE_END

#endif // !X_GUI_WINDOW_H_
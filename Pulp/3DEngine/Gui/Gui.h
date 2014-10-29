#pragma once

#ifndef X_GUI_H_
#define X_GUI_H_

#include <IGui.h>

X_NAMESPACE_BEGIN(gui)

class XWindow;

// this is basically a menu.
class XGui : public IGui
{
public:
	XGui();
	~XGui() X_FINAL;

	const char*	getName(void) const X_FINAL;

	void setCursorPos(float x, float y) X_FINAL;
	void setCursorPos(const Vec2f& pos) X_FINAL;
	Vec2f getCursorPos(void) X_FINAL;
	float getCursorPosX(void) X_FINAL;
	float getCursorPosY(void) X_FINAL;

	// repaints the ui
	void Redraw(int time, bool hud = false) X_FINAL;
	void DrawCursor(void) X_FINAL;

	const char* Activate(bool activate, int time) X_FINAL;

public:

	// Init this object with the contents of the file.
	bool InitFromFile(const char* name);

	bool ParseTextFile(const char* begin, const char* end);


private:

	bool isDeskTopValid(void) const;

private:
	core::string name_;
	Vec2f cursorPos_;

	XWindow*	pDesktop_;
};

#include "Gui.inl"

X_NAMESPACE_END

#endif // !X_GUI_H_
#pragma once


#ifndef X_GUI_H_I_
#define X_GUI_H_I_

#include <Math\XVector.h>

struct IGui
{
	virtual ~IGui() {};

	virtual const char*	getName() const X_ABSTRACT;

	// repaints the ui
	virtual void Redraw(int time, bool hud = false) X_ABSTRACT;
	virtual void DrawCursor() X_ABSTRACT;

	virtual const char *		Activate(bool activate, int time) X_ABSTRACT;

	// Triggers the gui and runs the onTrigger scripts.
	virtual void				Trigger(int time) = 0;


	virtual void setCursorPos(float x, float y) X_ABSTRACT;
	virtual Vec2f getCursorPos(void) X_ABSTRACT;
	virtual float getCursorPosX(void) X_ABSTRACT;
	virtual float getCursorPosY(void) X_ABSTRACT;
};


struct IGuiManger
{
	virtual ~IGuiManger() {};

	virtual void Init() X_ABSTRACT;
	virtual void Shutdown() X_ABSTRACT;



};


#endif // !X_GUI_H_I_
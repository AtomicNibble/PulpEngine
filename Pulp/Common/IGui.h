#pragma once


#ifndef X_GUI_H_I_
#define X_GUI_H_I_

#include <Math\XVector.h>

//
//	===== GUI =====
//
//	This is a custom gui system.
//
//	It will work on menu files that define a menu's layout and items.
//
//	the order of the items is the order they are drawn in.
//
//	each frame stuff that is been draw is added into a Vb/ IB
//	we allow diffrent shapes to be made like rectangles and others made from verts.
//
//	since we are sending verts for rectangles, supporting other shapes is easy.
//	Lines are also allowed.
//
//	I want to store everything in screen space for 2d menu's
//
//	what about menu's that are in the 3d world?
//
//	I could just take the size of them 3d menu items and scale them into the area.
//	instead of the screen space.
//
//	But what if i want a 3d menu that is not scaled, but can be any size.
//
//	will work it out later :Z
//
//	What about using swf? it means i only have to render and not do goaty things like 
//	manage the layout of shit.
//
//
//	How should i draw the gui?
//  Since somethings will be visable some frames, and also move around between frames.
//
//	So I think it's probs best if each frame i create a vb / IB in system memory.
//	It can be done as a job since it has real dependancies.
//

X_NAMESPACE_BEGIN(gui)

static const char*	GUI_FILE_EXTENSION = ".gui";
static const char*  GUI_BINARY_FILE_EXTENSION = ".guib";

// some limits
static const uint32_t GUI_MAX_MENUS = 64;
static const uint32_t GUI_MENU_MAX_ITEMS = 512; // max per a menu

struct IGui
{
	virtual ~IGui() {};

	virtual const char*	getName(void) const X_ABSTRACT;

	virtual void setCursorPos(float x, float y) X_ABSTRACT;
	virtual	void setCursorPos(const Vec2f& pos) X_ABSTRACT;
	virtual Vec2f getCursorPos(void) X_ABSTRACT;
	virtual float getCursorPosX(void) X_ABSTRACT;
	virtual float getCursorPosY(void) X_ABSTRACT;

	// repaints the ui
	virtual void Redraw(int time, bool hud = false) X_ABSTRACT;
	virtual void DrawCursor(void) X_ABSTRACT;

	// dose shit.
	virtual const char* Activate(bool activate, int time) X_ABSTRACT;
};


struct IGuiManger
{
	virtual ~IGuiManger() {};

	virtual void Init(void) X_ABSTRACT;
	virtual void Shutdown(void) X_ABSTRACT;



};

X_NAMESPACE_END

#endif // !X_GUI_H_I_
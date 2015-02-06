#pragma once

#ifndef X_GUI_H_
#define X_GUI_H_

#include "EngineBase.h"

#include <IGui.h>
#include <IInput.h>



X_NAMESPACE_BEGIN(gui)

class XWindow;
class XGuiManager;

// This is a interface container.
// it has a baser menu which may have multiple childs menus.
class XGui : public IGui,
 public engine::XEngineBase
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
	void Redraw() X_FINAL;
	void DrawCursor(void) X_FINAL;

	const char* Activate(bool activate, int time) X_FINAL;

	// same as input interface but not. since we don't register them.
	bool OnInputEvent(const input::InputEvent& event);
	bool OnInputEventChar(const input::InputEvent& event);

protected:
	friend class XGuiManager;

	// Init this object with the contents of the file.
	bool InitFromFile(const char* name);
	bool SaveBinaryVersion(void);

	void setName(const char* name);
private:
	bool ParseBinaryFile(const FileHdr& hdr, core::XFile* pFile);
	bool ParseTextFile(const char* begin, const char* end);

	bool isDeskTopValid(void) const;

private:
	core::string name_;
	Vec2f cursorPos_;

	XWindow*	pDesktop_;
	uint32_t sourceCrc32_;
};

#include "Gui.inl"

X_NAMESPACE_END

#endif // !X_GUI_H_
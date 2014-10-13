#pragma once

#ifndef _X_CUSTOMFRAME_H_
#define _X_CUSTOMFRAME_H_


// #include <Types\Rectangle.h>

X_NAMESPACE_BEGIN(core)


class xFrame
{
public:

	static void Startup(void);
	static void Shutdown(void);

	xFrame();
	~xFrame();

	LRESULT DrawFrame( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

	void SetIcon(HICON hicon) {
		this->m_Icon = hicon;
	}

	struct FrameButton {
		FrameButton() : Focus(false), Locked(false) {}

		bool Draw;
		bool Focus;
		bool Locked;
		char Type;
	};

private:
	void NCPaint( HWND hWnd, HDC hDC, WPARAM wParam );
	LRESULT NCHitTest( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam );
	void NCButtonDown( HWND hwnd, ULONG message, WPARAM wparam, LPARAM lparam );

	void LoadButtonInfo( HWND hwnd );
	void PaintButtons( HWND hWnd, HDC hDC );
	void PaintButton( int Idx, FrameButton* but, HDC dc, Recti pos );
	void PaintCaption(HWND hWnd, HDC hDC);

	void DraWMenu(HWND hWnd, HDC hDC);

private:
	// Buttons
	FrameButton m_Buttons[4];

	LONG	nHozBorder;
	LONG	nVerBorder;
	LONG	nCaptionHeight;

	BOOL	m_Hasfocus;
	BOOL	m_HasCaption;
	BOOL	m_IsMax;

	LONG	m_ClientWidth;
	LONG	m_ClientHeight;
	LONG	m_width;
	LONG	m_height;

	LONG	m_CapOff;

	HICON   m_Icon;

private:

	static uint32_t s_numframes;
};


X_NAMESPACE_END

#endif // _X_CUSTOMFRAME_H_
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

    LRESULT DrawFrame(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void SetIcon(HICON hicon)
    {
        this->Icon_ = hicon;
    }

    struct FrameButton
    {
        FrameButton() :
            Draw(false),
            Focus(false),
            Locked(false),
            Type(-1)
        {
        }

        bool Draw;
        bool Focus;
        bool Locked;
        char Type;
    };

private:
    void NCPaint(HWND hWnd, HDC hDC, WPARAM wParam);
    LRESULT NCHitTest(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    void NCButtonDown(HWND hwnd, ULONG message, WPARAM wparam, LPARAM lparam);

    void LoadButtonInfo(HWND hwnd);
    void PaintButtons(HWND hWnd, HDC hDC);
    void PaintButton(int Idx, FrameButton* but, HDC dc, Recti pos);
    void PaintCaption(HWND hWnd, HDC hDC);

    void DraWMenu(HWND hWnd, HDC hDC);

private:
    // Buttons
    FrameButton Buttons_[4];

    LONG nHozBorder_;
    LONG nVerBorder_;
    LONG nCaptionHeight_;

    BOOL Hasfocus_;
    BOOL HasCaption_;
    BOOL IsMax_;

    LONG ClientWidth_;
    LONG ClientHeight_;
    LONG width_;
    LONG height_;

    LONG CapOff_;

    HICON Icon_;

private:
    static AtomicInt s_numframes;
};

X_NAMESPACE_END

#endif // _X_CUSTOMFRAME_H_
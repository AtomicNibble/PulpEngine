#include "EngineCommon.h"

#include <ICore.h>

#include "CustomFrame.h"

#include "Util\BitUtil.h"

#include "resource.h"

#include "Util\LastError.h"

#include "Window.h"

X_LINK_LIB("Msimg32");

X_NAMESPACE_BEGIN(core)

#define WM_NCUAHDRAWCAPTION 0x00AE
#define WM_NCUAHDRAWFRAME 0x00AF

namespace
{
#ifndef SRCCOPY
#define SRCCOPY (DWORD)0x00CC0020
#endif

    struct MemHDC
    {
        MemHDC(HWND hWnd, HDC hDC) :
            hWnd_(hWnd),
            hDC_(hDC)
        {
            ::GetWindowRect(hWnd, &Rect_);

            mem_hDC_ = ::CreateCompatibleDC(hDC);
            MemBitmap_ = ::CreateCompatibleBitmap(hDC,
                Rect_.right - Rect_.left, Rect_.bottom - Rect_.top);

            OldBitmap_ = (HBITMAP)::SelectObject(mem_hDC_, MemBitmap_);
        }

        void BitBlt(int x, int y, int width, int height)
        {
            ::BitBlt(hDC_,
                x, y,
                width, height,
                mem_hDC_,
                x, y,
                SRCCOPY);
        }

        ~MemHDC()
        {
            /*
			BitBlt(hDC_, 
				0,0,
				Rect_.right - Rect_.left, Rect_.bottom - Rect_.top, 
				mem_hDC_, 
				0,0,
				SRCCOPY);
			*/

            SelectObject(mem_hDC_, OldBitmap_);
            ::DeleteObject(MemBitmap_);
            ::DeleteDC(mem_hDC_);
        }

        X_INLINE HDC GetGDC()
        {
            return mem_hDC_;
        }

    private:
        RECT Rect_;
        HBITMAP MemBitmap_;
        HBITMAP OldBitmap_;
        HWND hWnd_;
        HDC hDC_;
        HDC mem_hDC_;
    };

    template<class To, class From, int N>
    To GetByte(From Var)
    {
        return safe_static_cast<To, BYTE>(((Var >> (8 * N)) & 0xff));
    }

    class xRGB16
    {
    public:
        xRGB16()
        {
        }
        xRGB16(BYTE _r, BYTE _g, BYTE _b)
        {
            r = _r;
            g = _g;
            b = _b;
            Scale();
        }
        xRGB16(DWORD rgb)
        {
            r = GetByte<COLOR16, DWORD, 2>(rgb);
            g = GetByte<COLOR16, DWORD, 1>(rgb);
            b = GetByte<COLOR16, DWORD, 0>(rgb);
            Scale();
        }

        COLOR16 r, g, b;

    private:
        X_INLINE void Scale()
        {
            r = 255 * r;
            g = 255 * g;
            b = 255 * b;
        }
    };

    class xRGB
    {
    public:
        xRGB()
        {
        }
        xRGB(DWORD rgb)
        {
            col[0] = GetByte<BYTE, DWORD, 3>(rgb);
            col[1] = GetByte<BYTE, DWORD, 2>(rgb);
            col[2] = GetByte<BYTE, DWORD, 1>(rgb);
            col[3] = GetByte<BYTE, DWORD, 0>(rgb);
        }

        operator DWORD()
        {
            return rgba;
        }

        BYTE operator[](int i)
        {
            return col[i];
        }

    public:
        union
        {
            BYTE col[4];
            DWORD rgba;
        };
    };

    template<class T>
    inline POINT SetPoint(T x, T y)
    {
        POINT p;
        p.x = x;
        p.y = y;
        return p;
    }

    inline void SetVertex(TRIVERTEX& v, LONG x, LONG y, COLOR16 r, COLOR16 g, COLOR16 b, COLOR16 a)
    {
        v.x = x;
        v.y = y;
        v.Red = r;
        v.Green = g;
        v.Blue = b;
        v.Alpha = a;
    }

    struct FrameBorder
    {
        xRGB16 TopL;
        xRGB16 TopR;
        xRGB16 BottomL;
        xRGB16 BottomR;
    };

    struct PinStripe
    {
        xRGB Top;
        xRGB Bottom;
    };

    struct FrameColor
    {
        FrameBorder Frame;
        PinStripe Pin;
        xRGB Text;
    };

    FrameColor g_FrameActive;
    FrameColor g_FrameInActive;

    HICON g_But_Close[3];
    HICON g_But_Max[2];
    HICON g_But_Min[2];
    HICON g_But_Restore[2];

    static const DWORD TOM_PAINT_BUTTONS = WM_USER + 20;

    static HFONT g_font;
    static HICON g_hIcon;

    static HPEN g_pen;
    static HBRUSH g_Background;

    VOID WINAPI DrawRectangleGrad(HDC hdc, LONG x, LONG y, LONG width, LONG height, xRGB16 start, xRGB16 end, int flag)
    {
        TRIVERTEX vertex[2];
        GRADIENT_RECT gRect;

        gRect.UpperLeft = 0;
        gRect.LowerRight = 1;

        SetVertex(vertex[0], x, y, start.r, start.g, start.b, 0x0);
        SetVertex(vertex[1], x + width, y + height, end.r, end.g, end.b, 0x0);

        GradientFill(hdc, vertex, 2, &gRect, 1, flag);
    }

    VOID WINAPI DrawLine(HDC hdc, LONG x, LONG y, LONG width, LONG height, COLORREF bg)
    {
        HBRUSH brush = CreateSolidBrush(bg);

        HPEN old_pen = (HPEN)SelectObject(hdc, g_pen);
        HBRUSH old_bru = (HBRUSH)SelectObject(hdc, brush);

        ::Rectangle(hdc, x, y, x + width + 1, y + height + 1);

        ::SelectObject(hdc, old_pen);
        ::SelectObject(hdc, old_bru);

        ::DeleteObject(brush);
    }

} // namespace

#define LOAD_FRAME_ICON(Arr, ID)   \
    Arr[0] = _LoadIcon(ID##_DULL); \
    Arr[1] = _LoadIcon(ID);

#define FREE_FRAME_ICON(Arr) \
    DestroyIcon(Arr[0]);     \
    DestroyIcon(Arr[1]);

HICON _LoadIcon(int ID)
{
    HICON icon = (HICON)::LoadImage(
        ::GetModuleHandle(NULL),
        MAKEINTRESOURCE(ID),
        IMAGE_ICON,
        0,
        0,
        0);

    if (!icon) {
        lastError::Description Dsc;
        X_ERROR("WindowFrame", "Cannot load icon. ID:%i -> Error: %s", ID, lastError::ToString(Dsc));
    }

    return icon;
}

AtomicInt xFrame::s_numframes(0);

void xFrame::Startup(void)
{
    g_FrameActive.Frame.TopL = 0x444444;
    g_FrameActive.Frame.TopR = 0x282828;
    g_FrameActive.Frame.BottomL = 0x2c2c2c;
    g_FrameActive.Frame.BottomR = 0x3c3c3c;

    g_FrameActive.Pin.Top = 0x101010; // 0x101010
    g_FrameActive.Pin.Bottom = 0x515151;

    g_FrameActive.Text = 0xaaaaaaaa;

    // ----------------------------------

    g_FrameInActive.Frame.TopL = 0x444444;
    g_FrameInActive.Frame.TopR = 0x282828;
    g_FrameInActive.Frame.BottomL = 0x2c2c2c;
    g_FrameInActive.Frame.BottomR = 0x3c3c3c;

    g_FrameInActive.Pin.Top = 0x101010;
    g_FrameInActive.Pin.Bottom = 0x515151;

    g_FrameInActive.Text = 0xa0a0a0a0;

    // Load icons

    LOAD_FRAME_ICON(g_But_Close, IDI_FRAME_CLOSE);
    LOAD_FRAME_ICON(g_But_Max, IDI_FRAME_MAX);
    LOAD_FRAME_ICON(g_But_Min, IDI_FRAME_MIN);
    LOAD_FRAME_ICON(g_But_Restore, IDI_FRAME_RESTORE);

    g_But_Close[2] = _LoadIcon(IDI_FRAME_CLOSE_DISABLE);

    g_hIcon = _LoadIcon(IDI_ENGINE_LOGO);

    g_pen = CreatePen(PS_NULL, 1, RGB(90, 90, 90));

    g_Background = CreateSolidBrush(0x202020);
    g_font = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_ROMAN, L"Times New Roman");
}

void xFrame::Shutdown(void)
{
    FREE_FRAME_ICON(g_But_Close);
    FREE_FRAME_ICON(g_But_Max);
    FREE_FRAME_ICON(g_But_Min);
    FREE_FRAME_ICON(g_But_Restore);

    DestroyIcon(g_But_Close[2]);

    DeleteObject(g_font);
    DeleteObject(g_Background);
    DeleteObject(g_pen);
}

/// ------------------------------------------------------------------------------------------

xFrame::xFrame()
{
    if (s_numframes == 0) {
        Startup();
    }
    ++s_numframes;

    nHozBorder_ = ::GetSystemMetrics(SM_CXSIZEFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER);
    nVerBorder_ = ::GetSystemMetrics(SM_CYSIZEFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER);
    nCaptionHeight_ = ::GetSystemMetrics(SM_CYCAPTION);

    Buttons_[0].Type = HTMENU;
    Buttons_[1].Type = HTGROWBOX;
    Buttons_[2].Type = HTGROWBOX;
    Buttons_[3].Type = HTMINBUTTON;

    Hasfocus_ = FALSE;
    HasCaption_ = -1;

    CapOff_ = 0;

    IsMax_ = FALSE;

    Icon_ = NULL;
}

xFrame::~xFrame()
{
    --s_numframes;
    if (s_numframes == 0) {
        Shutdown();
    }
}

LRESULT xFrame::DrawFrame(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_ERASEBKGND: // prevent this msg.
        {
            HDC hdc = (HDC)wParam;

            RECT crect;
            GetClientRect(hWnd, &crect);
            FillRect(hdc, &crect, g_Background);
            return TRUE;
        }
        case WM_NCACTIVATE: {
            if (wParam == FALSE || wParam == TRUE)
                return 1;
            break;
        }

        case WM_NCPAINT:
        case WM_NCUAHDRAWFRAME: {
            LoadButtonInfo(hWnd);

            HDC hdc = GetWindowDC(hWnd);

            //	DraWMenu(hWnd, hdc);

            NCPaint(hWnd, hdc, wParam);

            ReleaseDC(hWnd, hdc);
            return 0;
        }
        case WM_NCUAHDRAWCAPTION: {
            HDC hdc = GetWindowDC(hWnd);
            PaintCaption(hWnd, hdc);
            ReleaseDC(hWnd, hdc);
            return 0;
        }
        case TOM_PAINT_BUTTONS: {
            HDC hdc = GetWindowDC(hWnd);
            PaintButtons(hWnd, hdc);
            ReleaseDC(hWnd, hdc);
            return 0;
        }
        case WM_ACTIVATE: {
            if (wParam == WA_INACTIVE)
                Hasfocus_ = 0;
            else
                Hasfocus_ = 1;

            SendMessage(hWnd, WM_NCPAINT, MAKEWPARAM(0, 0), 0);
            break;
        }
        case WM_NCHITTEST: {
            return NCHitTest(hWnd, msg, wParam, lParam);
        }
        case WM_NCLBUTTONDOWN: {
            NCButtonDown(hWnd, msg, wParam, lParam);
            break;
        }

        case WM_SIZE: {
            switch (wParam) {
                case SIZE_MAXIMIZED: {
                    if (!IsMax_) {
                        IsMax_ = TRUE;
                        CapOff_ = 2;

                        Buttons_[1].Draw = false;
                        Buttons_[2].Draw = true;
                    }
                    break;
                }
                case SIZE_RESTORED: {
                    if (IsMax_) {
                        IsMax_ = FALSE;
                        CapOff_ = 0;

                        Buttons_[1].Draw = true;
                        Buttons_[2].Draw = false;
                    }
                    break;
                }
            }
            break;
        }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

/// ------------------------------------------------------------------------------------------

enum
{ // Index defines
    BUTTON_EXIT,
    BUTTON_MAX,
    BUTTON_RESTORE,
    BUTTON_MIN
};

void xFrame::LoadButtonInfo(HWND hwnd)
{
    if (HasCaption_ == -1) {
        LONG Styles = GetWindowLong(hwnd, GWL_STYLE);

        HasCaption_ = bitUtil::IsBitFlagSet(Styles, WS_CAPTION | WS_SYSMENU);

        if (HasCaption_) {
            Buttons_[BUTTON_EXIT].Draw = true;
            Buttons_[BUTTON_MIN].Draw = bitUtil::IsBitFlagSet(Styles, WS_MINIMIZEBOX);

            if (bitUtil::IsBitFlagSet(Styles, WS_MAXIMIZEBOX)) {
                Buttons_[BUTTON_MAX].Draw = true;
                Buttons_[BUTTON_RESTORE].Draw = false;
            }
        }
    }
}

void xFrame::NCPaint(HWND hWnd, HDC hDC_, WPARAM wParam)
{
    RECT rcClient, rcWind;
    POINT temp;

    X_UNUSED(wParam);

    GetClientRect(hWnd, &rcClient);
    GetWindowRect(hWnd, &rcWind);

    temp.x = rcClient.top;
    temp.y = rcClient.left;

    ClientToScreen(hWnd, &temp);

    LONG CaptionHeight = nCaptionHeight_ + (HasCaption_ ? nHozBorder_ : 0);

    LONG CaptionEnd = temp.y - rcWind.top;

    ClientWidth_ = rcClient.right - rcClient.left;
    ClientHeight_ = rcClient.bottom - rcClient.top;

    width_ = (rcWind.right - rcWind.left);
    height_ = (rcWind.bottom - rcWind.top);

    MemHDC mem(hWnd, hDC_);

    HDC hDC = mem.GetGDC();

    FrameColor* color = Hasfocus_ ? &g_FrameActive : &g_FrameInActive;

    // Top Frame
    DrawRectangleGrad(
        hDC,
        0, 0, width_, CaptionHeight,
        color->Frame.TopL,
        color->Frame.TopR,
        GRADIENT_FILL_RECT_H);

    // left frame
    DrawRectangleGrad(
        hDC,
        0, 0, nVerBorder_, height_,
        color->Frame.TopL,
        color->Frame.BottomL,
        GRADIENT_FILL_RECT_V);

    // right frame
    DrawRectangleGrad(
        hDC,
        width_ - nVerBorder_, 0, width_, height_,
        color->Frame.TopR,
        color->Frame.BottomR,
        GRADIENT_FILL_RECT_V);
    // bottom frame
    DrawRectangleGrad(
        hDC,
        0, height_ - nHozBorder_, width_, height_,
        color->Frame.BottomL,
        color->Frame.BottomR,
        GRADIENT_FILL_RECT_H);

    // I want to do a sunken look, so darker for top & left
    DWORD col_dark = color->Pin.Top;
    DWORD col = color->Pin.Bottom;

    // Top
    DrawLine(hDC, nHozBorder_, CaptionEnd - 1, ClientWidth_ + 1, 1, col_dark);

    // Left
    DrawLine(hDC, nHozBorder_ - 1, CaptionEnd, 1, ClientHeight_ + 1, col_dark);

    // Right
    DrawLine(hDC, width_ - (nHozBorder_), CaptionEnd, 1, ClientHeight_ + 1, col);

    // Bottom
    DrawLine(hDC, nHozBorder_ - 1, height_ - (nVerBorder_), ClientWidth_ + 2, 1, col);

    if (HasCaption_) {
        // draw scaled icon slut
        DrawIconEx(hDC, 9, nHozBorder_ + CapOff_, Icon_ ? Icon_ : g_hIcon, 16, 16, 0, NULL, DI_NORMAL);

        PaintCaption(hWnd, hDC);

        // Buttons
        PaintButtons(hWnd, hDC);
    }

    // only BitBlt the shit we painted.
    {
        // Top
        mem.BitBlt(0, 0, width_, CaptionHeight);
        // Left
        mem.BitBlt(0, 0, nVerBorder_, height_);
        // right
        mem.BitBlt(width_ - nVerBorder_, 0, width_, height_);
        // Bottom
        mem.BitBlt(0, height_ - nHozBorder_, width_, height_);
    }
}

xRGB16 blend(xRGB16 start, xRGB16 end, float percent)
{
    xRGB16 out;

    float alpha = 1.f - percent;
    float inv_alpha = percent;

    out.r = (COLOR16)((alpha * (float)start.r) + (inv_alpha * (float)end.r));
    out.g = (COLOR16)((alpha * (float)start.g) + (inv_alpha * (float)end.g));
    out.b = (COLOR16)((alpha * (float)start.b) + (inv_alpha * (float)end.b));

    return out;
}

void xFrame::PaintCaption(HWND hWnd, HDC hDC)
{
    FrameColor* color = Hasfocus_ ? &g_FrameActive : &g_FrameInActive;

    LONG CaptionHeight = nCaptionHeight_ + (HasCaption_ ? nHozBorder_ : 0);

    //	DWORD col_dark = color->Pin.Top;
    //	DWORD col = color->Pin.Bottom;

    if (HasCaption_) {
        StackStringW512 Title;

        int IconPad = 16 + 4;
        int Len = GetWindowTextW(hWnd, &Title[0], 512);

        // Title
        SetBkMode(hDC, TRANSPARENT);
        SetTextColor(hDC, color->Text);
        SelectObject(hDC, g_font);

        RECT size;
        size.left = nVerBorder_ + 2 + IconPad;
        size.top = nHozBorder_ + CapOff_;
        size.bottom = CaptionHeight - 2;
        size.right = width_ - 70;

        // can i blend the colors myself?
        // well will it look correct is the question
        // if we have a start color and end color and a range.
        // we can just blend
        int real_wdith = width_;
        int innder_width = size.right - size.left;

        float left_offset = (float)size.left / (float)real_wdith;
        float right_offset = (float)(size.left + innder_width) / (float)real_wdith;

        xRGB16 new_left = blend(color->Frame.TopL, color->Frame.TopR, left_offset);
        xRGB16 new_right = blend(color->Frame.TopL, color->Frame.TopR, right_offset);

        // Top Frame
        DrawRectangleGrad(
            hDC,
            size.left, 0, innder_width, CaptionHeight - 1,
            new_left,
            new_right,
            GRADIENT_FILL_RECT_H);

        DrawTextExW(hDC, &Title[0], Len, &size, DT_END_ELLIPSIS | DT_SINGLELINE, 0);

        SetBkMode(hDC, OPAQUE);
    }
}

#define BUTTON_WIDTH 14
#define BUTTON_HEIGHT 16
#define TOP_MARGIN 3
#define RIGHT_MARGIN BUTTON_WIDTH + 8

#define SPACING (BUTTON_WIDTH + 5)

inline Recti GetButtonPos(int index, int width, BOOL max)
{
    int start = width - (RIGHT_MARGIN + (index * SPACING));
    return Recti(
        start,
        TOP_MARGIN + (max ? 6 : 0),
        start + BUTTON_WIDTH,
        BUTTON_HEIGHT);
}

void xFrame::PaintButtons(HWND hWnd, HDC hDC)
{
    int Drawn = 0;
    X_UNUSED(hWnd);

    for (int i = 0; i < 4; i++) {
        if (Buttons_[i].Draw) {
            PaintButton(i, &Buttons_[i], hDC, GetButtonPos(Drawn, width_, IsMax_));
            Drawn++;
        }
    }
}

void xFrame::PaintButton(int Idx, FrameButton* but, HDC dc, Recti rec)
{
    Vec2i pos = rec.getUpperLeft();

    if (but->Locked) {
        but->Focus = false;
    }

    int Dull = static_cast<int>(but->Focus);

    HICON Img = NULL;
    switch (Idx) {
        case 0: // Exit
            if (but->Locked)
                Dull = 2;
            Img = g_But_Close[Dull];
            break;
        case 1: // Max
            Img = g_But_Max[Dull];
            break;
        case 2: // restore
            Img = g_But_Restore[Dull];
            break;
        case 3: // min
            Img = g_But_Min[Dull];
            break;
        default:
            X_ASSERT_UNREACHABLE();
            break;
    }

    if (Img) {
        DrawIconEx(dc, pos.x, pos.y, Img, 16, 16, 0, NULL, DI_NORMAL);
    }
}

LRESULT xFrame::NCHitTest(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    RECT rect;
    GetWindowRect(hwnd, &rect);

    LONG CaptionHeight = nCaptionHeight_ + (HasCaption_ ? nHozBorder_ : 0);

    Vec2i mouse(LOWORD(lparam) - rect.left, HIWORD(lparam) - rect.top);

    Recti frame(0, 0, rect.right - rect.left, rect.bottom - rect.top);
    Recti c_frame(nVerBorder_, CaptionHeight, ClientWidth_, ClientHeight_);
    Recti caption(0, 0, ClientWidth_, 4);
    Recti top_left(0, 0, 4, 4);
    Recti top_right(ClientWidth_ + nHozBorder_, 0, ClientWidth_ + nHozBorder_ + nHozBorder_, 4);
    Recti left(0, 0, 4, CaptionHeight);
    Recti right(ClientWidth_ + nHozBorder_, 0, ClientWidth_ + nHozBorder_ + nHozBorder_, CaptionHeight);

    bool Focus = true;
    LRESULT res = HTNOWHERE;

    for (int32_t i = 0; i < 4; i++) {
        if (Buttons_[i].Focus) {
            Buttons_[i].Focus = false;
            Focus = false;
        }
    }

    if (c_frame.contains(mouse))
        res = HTCLIENT;
    else if (top_left.contains(mouse))
        res = HTTOPLEFT;
    else if (top_right.contains(mouse))
        res = HTTOPRIGHT;
    else if (left.contains(mouse))
        res = HTLEFT;
    else if (right.contains(mouse))
        res = HTRIGHT;
    else if (caption.contains(mouse))
        res = HTTOP;

    if (res != HTNOWHERE) {
        if (Focus) {
            SendMessage(hwnd, TOM_PAINT_BUTTONS, MAKEWPARAM(0, 0), 0);
        }
        return res;
    }

    int enabled = 0;
    for (int32_t i = 0; i < 4; i++) {
        if (!Buttons_[i].Draw) {
            continue;
        }

        //	bool temp_focus = Buttons_[i].Focus;
        if (GetButtonPos(enabled, width_, IsMax_).contains(mouse)) {
            Buttons_[i].Focus = true;
            //		if(!temp_focus)
            SendMessage(hwnd, TOM_PAINT_BUTTONS, MAKEWPARAM(0, 0), 0);
            return Buttons_[i].Type;
        }
        enabled++;
        //	Buttons_[i].Focus = false;
        //	if(temp_focus)
        //		SendMessage(hwnd, TOM_PAINT_BUTTONS, MAKEWPARAM(0, 0), 0);
    }

    // title bar
    if (Recti(0, 0, width_, CaptionHeight).contains(mouse))
        return HTCAPTION;

    return DefWindowProc(hwnd, message, wparam, lparam);
}

void xFrame::NCButtonDown(HWND hwnd, ULONG message, WPARAM wparam, LPARAM lparam)
{
    RECT rect;
    GetWindowRect(hwnd, &rect);

    Vec2<int32_t> mouse(LOWORD(lparam) - rect.left, HIWORD(lparam) - rect.top);

    X_UNUSED(message);
    X_UNUSED(wparam);

    int enabled = 0;
    for (int32_t i = 0; i < 4; i++) {
        if (!Buttons_[i].Draw || Buttons_[i].Locked) {
            continue;
        }

        if (GetButtonPos(enabled, width_, IsMax_).contains(mouse)) {
            xWindow* pWindow = reinterpret_cast<xWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

            switch (Buttons_[i].Type) {
                case HTMENU:
                    pWindow->Close();
                    break;
                case HTGROWBOX: // HTMAXBUTTON:
                {
                    if (i == 1 && (i + 1) < sizeof(Buttons_) / sizeof(Buttons_[0])) {
                        pWindow->Maximise();
                        Buttons_[i + 1].Draw = true;
                    }
                    else if (i > 0) {
                        pWindow->Restore();
                        Buttons_[i - 1].Draw = true;
                    }

                    Buttons_[i].Draw = false;
                    break;
                }
                case HTMINBUTTON:
                    pWindow->Minamise();
                    break;
            }
            goto paint;
        }
        enabled++;
    }
    return;
paint:
    SendMessage(hwnd, WM_NCHITTEST, MAKEWPARAM(0, 0), 0);
    SendMessage(hwnd, TOM_PAINT_BUTTONS, MAKEWPARAM(0, 0), 0);
}

X_NAMESPACE_END
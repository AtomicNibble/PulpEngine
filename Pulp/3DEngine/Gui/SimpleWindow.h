#pragma once

#ifndef X_GUI_SIMPLE_WINDOW_H_
#define X_GUI_SIMPLE_WINDOW_H_

X_NAMESPACE_BEGIN(engine)

namespace gui
{
    class XWindowSimple
    {
        friend class XWindow;

    public:
        XWindowSimple();
        ~XWindowSimple();

        void drawBackground(const Rectf& drawRect);
        void drawBorderAndCaption(const Rectf& drawRect);

        X_INLINE const char* getName(void) const;
        X_INLINE XWindow* getParent(void);

    private:
        core::string name_;
        core::string text_;

        Rectf rect_; // overall rect
        Color backColor_;
        Color matColor_;
        Color foreColor_;
        Color borderColor_;
        float textScale_;
        bool visible_;

        XWindow* pParent_;
    };

#include "SimpleWindow.inl"

} // namespace gui

X_NAMESPACE_END

#endif // !X_GUI_SIMPLE_WINDOW_H_
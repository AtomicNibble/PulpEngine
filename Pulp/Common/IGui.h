#pragma once

#ifndef X_GUI_H_I_
#define X_GUI_H_I_

#include <Math\XVector.h>


X_NAMESPACE_DECLARE(engine,
                    class IPrimativeContext);

X_NAMESPACE_BEGIN(engine)

namespace gui
{
    static const char* MENU_FILE_EXTENSION = "lua";

    static const uint32_t MENU_MAX_LOADED = 64;


#if 0
    X_DECLARE_FLAGS(WindowFlag)
    (
        CAPTION,
        DESKTOP, // base window.
        CHILD,
        BORDER, // has a border, cyptic var name i know. (insert camel ascii here)
        SIZABLE,
        MOVEABLE,
        FOCUS,
        SELECTED,
        NOCURSOR,
        ACTIVE,
        MODAL, // eats events, instead of passing to children.
        FULLSCREEN,
        NO_CLIP,
        NO_CURSOR,
        IN_TRANSITION);

    X_DECLARE_ENUM(WindowStyle)
    (
        EMPTY,      // no background
        FILLED,     // filled with background color
        GRADIENT,   // gradient bar based on background color
        SHADER,     // material mixed with background color
        DVAR_SHADER // draws the material specified by the dvar
    );

    X_DECLARE_ENUM(WindowBorderStyle)
    (
        NONE,
        FULL,     // full border based on border color ( single pixel )
        HORZ,     // horizontal borders only
        VERT,     // vertical borders only
        GRADIENT, // horizontal border using the gradient bars
        RAISED,   // darken the bottom and right sides of the border
        SUNKEN    // darken the top and left sides of the border
    );

    // adds some type saftey.
    // int values must be cast.
    struct TextAlign
    {
        // 2|2
        enum Enum : unsigned char
        {
            LEFT = 0x0,
            CENTER = 0x1,
            RIGHT = 0x2,

            // bits 2&3
            TOP = 0x4,
            MIDDLE = 0x8,
            BOTTOM = 0xc,

            // masks
            HOZ_MASK = 0x2,
            VERT_MASK = 0x0c,
        };

        struct Bits
        {
            uint32_t BIT_hozalign : 2;
            uint32_t BIT_vertAlign : 2;
        };

        typedef uint32_t Value;

        // defaults to left align since left is 0x0.
        static const Enum TOP_LEFT = (Enum)(TOP | LEFT);
        static const Enum TOP_CENTER = (Enum)(TOP | CENTER);
        static const Enum TOP_RIGHT = (Enum)(TOP | RIGHT);

        static const Enum MIDDLE_LEFT = (Enum)(MIDDLE | LEFT);
        static const Enum MIDDLE_CENTER = (Enum)(MIDDLE | CENTER);
        static const Enum MIDDLE_RIGHT = (Enum)(MIDDLE | RIGHT);

        static const Enum BOTTOM_LEFT = (Enum)(BOTTOM | LEFT);
        static const Enum BOTTOM_CENTER = (Enum)(BOTTOM | CENTER);
        static const Enum BOTTOM_RIGHT = (Enum)(BOTTOM | RIGHT);

        // max the value can be.
        static const uint32_t MAX_VALUE = BOTTOM_RIGHT;
    };

#endif

    struct IGui
    {
        virtual ~IGui() = default;

        virtual void Draw(engine::IPrimativeContext* pDrawCon) X_ABSTRACT;
    };

    struct IGuiManger
    {
        virtual ~IGuiManger() = default;

        virtual bool init(void) X_ABSTRACT;
        virtual void shutdown(void) X_ABSTRACT;

        virtual IGui* loadGui(const char* pName) X_ABSTRACT;
        virtual IGui* findGui(const char* pName) X_ABSTRACT;

        virtual void listGuis(const char* pWildcardSearch = nullptr) const X_ABSTRACT;
    };

} // namespace gui

X_NAMESPACE_END

#endif // !X_GUI_H_I_

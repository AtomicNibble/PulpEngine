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

X_NAMESPACE_DECLARE(engine,
                    class IPrimativeContext);

X_NAMESPACE_BEGIN(engine)

namespace gui
{
    static const char* MENU_FILE_EXTENSION = "lua";
    static const char* GUI_FILE_EXTENSION = "gui";
    static const char* GUI_BINARY_FILE_EXTENSION = "guib";

    static const uint32_t GUI_BINARY_MAGIC = X_TAG('g', 'u', 'i', 'b');
    static const uint8_t GUI_BINARY_VERSION = 1;

    // some limits
    static const uint32_t GUI_MAX_MENUS = 64;
    static const uint32_t GUI_MENU_MAX_ITEMS = 512; // max per a menu

    static const uint32_t GUI_CAPTION_HEIGHT = 16;
    static const uint32_t GUI_SCROLLER_SIZE = 16;
    static const uint32_t GUI_SCROLLBAR_SIZE = 16;

    static const uint32_t GUI_MAX_WINDOW_NAME_LEN = 28; // error on longer, instead of just clipping.
    static const uint32_t GUI_MAX_LIST_ITEMS = 1024;

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

    struct FileHdr
    {
        FileHdr()
        {
            core::zero_this(this);
        }

        uint32_t Magic;
        uint8_t version;
        uint8_t pad[3];
        uint32_t crc32;
        uint32_t fileSize;

        X_INLINE bool IsValid(void) const
        {
            return Magic == GUI_BINARY_MAGIC;
        }

        X_INLINE bool IsCurrentVersion(void) const
        {
            return version == GUI_BINARY_VERSION;
        }
    };

    X_ENSURE_SIZE(FileHdr, 16)
    // i won't have a seprate gui flag.
    // i'll just implment a seralise methord in the window class.

    struct IGui
    {
        virtual ~IGui(){};

        virtual const char* getName(void) const X_ABSTRACT;

        virtual void setCursorPos(float x, float y) X_ABSTRACT;
        virtual void setCursorPos(const Vec2f& pos) X_ABSTRACT;
        virtual Vec2f getCursorPos(void) X_ABSTRACT;
        virtual float getCursorPosX(void) X_ABSTRACT;
        virtual float getCursorPosY(void) X_ABSTRACT;

        // repaints the ui
        virtual void Redraw(engine::IPrimativeContext* pDrawCon) X_ABSTRACT;
        virtual void DrawCursor(engine::IPrimativeContext* pDrawCon) X_ABSTRACT;

        // dose shit.
        virtual const char* Activate(bool activate, int time) X_ABSTRACT;
    };

    struct IGuiManger
    {
        virtual ~IGuiManger(){};

        virtual bool init(void) X_ABSTRACT;
        virtual void shutdown(void) X_ABSTRACT;

        virtual IGui* loadGui(const char* name) X_ABSTRACT;
        virtual IGui* findGui(const char* name) X_ABSTRACT;

        virtual void listGuis(const char* wildcardSearch = nullptr) const X_ABSTRACT;
    };

} // namespace gui

X_NAMESPACE_END

#endif // !X_GUI_H_I_

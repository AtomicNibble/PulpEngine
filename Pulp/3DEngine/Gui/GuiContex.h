#pragma once


#include <Time\TimeVal.h>

#include <IFont.h>

X_NAMESPACE_DECLARE(core, struct FrameInput)


X_NAMESPACE_BEGIN(engine)

class PrimativeContext;

namespace gui
{
   
    class GuiContex
    {
        X_DECLARE_ENUM(Mouse)(
            LEFT,
            RIGHT
        );

        using MouseDownArr = std::array<bool, Mouse::ENUM_COUNT>;
        using ItemWidthStack = core::FixedStack<float, 8>;

        typedef core::Hash::Fnv1aVal ItemID;

        static const ItemID INVALID_ITEM_ID = static_cast<ItemID>(0);

    public:
        struct Params
        {
            core::TimeVal frameDelta;
            Vec2f displaySize;
            Vec2f cursorPos;
            Rectf rect;
        };

        struct DrawCtx
        {
            Vec2f currentPos;

            ItemID lastItemID;
            Rectf lastItemRect;
        };

        struct Style
        {
            Vec2f framePadding;
            Vec2f itemSpacing;

            Color8u btnCol;
            Color8u btnHover;
            Color8u btnHold;

            Color8u borderCol;
            Color8u borderColForcus;
        };

    public:
        GuiContex();

        void init(engine::Material* pCursor);
        void setPrimContet(PrimativeContext* pPrim);

        void processInput(core::FrameInput& input);

        void begin(Params& params);
        void end(void);

        void setFont(font::IFont* pFont);
        void fill(Color8u col);

        // layout
        void center(void);

        // item width
        void pushItemWidth(float width);
        void popItemWidth(void);

        // Buttons
        bool button(const char* pText);
        bool button(const char* pText, const char* pEnd);

        // Labels
        void text(const char* pText);

        // Sliders
        void slider(const char* pLabel, const char* pVarName);

    private:
        Vec2f calcTextSize(const char* pBegin, const char* pEnd);
        Vec2f calcItemSize(Vec2f size, Vec2f defaultSize);
        Vec2f calcItemSize(Vec2f size, float defaultX, float defaultY);

        void addItem(const Rectf& r, ItemID id);

        bool itemHoverable(ItemID id, const Rectf& r);

        void setActiveID(ItemID id);
        void clearActiveID(void);

        static ItemID getID(const char* pLabel);
        static ItemID getID(const char* pBegin, const char* pEnd);

    private:
        PrimativeContext* pPrim_;
        Rectf rect_;
        font::TextDrawContext txtCtx_;

        DrawCtx dc_;
        Style style_;

        MouseDownArr mouseDown_;

        ItemID activeId_;
        ItemID hoveredId_;

        Vec2f cursorPos_;
        Vec2f cursorDelta_;

        // items widths.
        float itemWidthDefault_;
        float itemWidth_;
        ItemWidthStack itemWidthStack_;

        engine::Material* pCursor_;
    };


} // namespace gui

X_NAMESPACE_END
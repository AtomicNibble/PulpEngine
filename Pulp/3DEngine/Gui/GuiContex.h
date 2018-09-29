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
            Vec2f indent;
            Vec2f currentPos;
            Vec2f currentPosStart;

            ItemID lastItemID;
            Rectf lastItemRect;
        };

        struct Style
        {
            Color8u getBorderCol(bool hovered) const {
                return hovered ? borderColFocus : borderCol;
            }

            Color8u getBackgroundCol(bool held, bool hovered) const {
                if (held) {
                    return backgroundHoldCol;
                }
                else if (hovered) {
                    return backgroundColFocus;
                }

                return backgroundCol;
            }

            Vec2f framePadding;
            Vec2f itemSpacing;
            Vec2f windowPadding;

            Color8u backgroundCol;
            Color8u backgroundColFocus;
            Color8u backgroundHoldCol;

            Color8u borderCol;
            Color8u borderColFocus;

            Color8u chkBoxCol;
            Color8u chkBoxFillCol;

            Color8u barBckCol;
            Color8u barFillCol;
        };

        X_DECLARE_FLAGS(WindowFlag)(
            Popup
        );

        typedef Flags<WindowFlag> WindowFlags;

        struct NextWindowData
        {
            Vec2f posVal;
            Vec2f sizeVal;
        };

        struct Window
        {
            typedef core::FixedStack<ItemID, 32> ItemIDStack;

        public:
            Window(PrimativeContext* pPrim, const char* pName);

            ItemID getID(const char* pLabel);
            ItemID getID(const char* pBegin, const char* pEnd);

        public:
            PrimativeContext* pPrim;
            core::StackString<32, char> name;
            ItemID ID;
            WindowFlags flags;
            bool active;

            int32_t lastActiveFrame;

            Vec2f pos;
            Vec2f size;

            ItemIDStack IDStack;

            DrawCtx dc;
        };

        struct PopupRef
        {
            ItemID popupId;
            ItemID openParentId;
            Window* pWindow;
            Vec2f openPopupPos;
            Vec2f openMousePos;
        };

        typedef core::FixedStack<Window*, 4> WindowPtrStack;
        typedef core::FixedArray<Window*, 4> WindowPtrArr;

        typedef core::FixedStack<PopupRef, 4> PopupRefStack;
        typedef core::FixedArray<PopupRef, 4> PopupRefArr;

    public:
        GuiContex();

        void init(engine::Material* pCursor, engine::Material* pSpinner);
        void setPrimContet(PrimativeContext* pPrim);

        void processInput(core::FrameInput& input);

        void beginFrame(Params& params);
        void endFrame(void);

        X_INLINE void setNextWindowPos(const Vec2f& pos);
        X_INLINE void setNextWindowSize(const Vec2f& size);

        void begin(const char* pName, WindowFlags flags);
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
        void text(const char* pText, const char* pEnd, Color8u col);

        // Sliders
        void slider(const char* pLabel, const char* pVarName, float increment);

        // checkbox
        void checkbox(const char* pLabel, const char* pVarName);

        // combo - (return true if changed)
        bool combo(const char* pLabel, core::span<const char*> items, int32_t& currentIdx);
        bool comboBegin(const char* pLabel, const char* pPreviewValue);
        void comboEnd(void);

        // selectables
        bool selectable(const char* pLabel, bool selected);

        // who's a good baby?
        void pacifier(float dt);

    private:
        Vec2f calcTextSize(const char* pBegin, const char* pEnd);
        Vec2f calcItemSize(Vec2f size, Vec2f defaultSize);
        Vec2f calcItemSize(Vec2f size, float defaultX, float defaultY);

        void addItem(const Rectf& r, ItemID id);

        bool itemHoverable(ItemID id, const Rectf& r);
        bool buttonBehavior(ItemID id, const Rectf& r, bool* pHovered, bool* pHeld);

        bool isPopupOpen(ItemID id) const;
        void openPopUp(ItemID id);
        void endPopUp(void);

        Window* findWindow(const char* pName) const;
        Window* createWindow(const char* pName, WindowFlags flags);
        void setCurrentWindow(Window* pWindow);

        void setActiveID(ItemID id);
        void clearActiveID(void);

        void pushID(ItemID id);
        void popID(void);

        ItemID getID(const char* pLabel) const;
        ItemID getID(const char* pBegin, const char* pEnd) const;

    private:
        PrimativeContext* pPrim_;
        Rectf screenRect_;
        font::TextDrawContext txtCtx_;

        Style style_;

        int32_t currentFrame_;

        MouseDownArr mouseDown_;

        Vec2f cursorPos_;
        Vec2f cursorDelta_;

        ItemID activeId_;
        ItemID hoveredId_;

        // items widths.
        float itemWidthDefault_;
        float itemWidth_;
        ItemWidthStack itemWidthStack_;

        // window
        NextWindowData nextWindowData_;

        WindowPtrArr windows_;
        WindowPtrStack currentWindowStack_;
        Window* pCurrentWindow;

        // popup
        PopupRefStack openPopupStack_;
        PopupRefStack currentPopupStack_;

        engine::Material* pCursor_;
        engine::Material* pSpinner_;
    };


    X_INLINE void GuiContex::setNextWindowPos(const Vec2f& pos)
    {
        nextWindowData_.posVal = pos;
    }

    X_INLINE void GuiContex::setNextWindowSize(const Vec2f& size)
    {
        nextWindowData_.sizeVal = size;
    }

} // namespace gui

X_NAMESPACE_END
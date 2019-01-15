#include "stdafx.h"
#include "GuiContex.h"

#include <IConsole.h>
#include <IInput.h>
#include <IFrameData.h>

#include <IGame.h>
#include <INetwork.h>

#include "Drawing\PrimativeContext.h"

X_NAMESPACE_BEGIN(engine)

namespace gui
{
    namespace
    {
        core::Hash::Fnv1aVal idHash(const char* pStr)
        {
            return core::Hash::Fnv1aHash(pStr, core::strUtil::strlen(pStr));
        }

        core::Hash::Fnv1aVal idHash(const char* pBegin, const char* pEnd)
        {
            return core::Hash::Fnv1aHash(pBegin, safe_static_cast<size_t>(pEnd - pBegin));
        }

        core::Hash::Fnv1aVal idHash(const char* pBegin, const char* pEnd, core::Hash::Fnv1aVal seed)
        {
            return core::Hash::Fnv1aHash(pBegin, safe_static_cast<size_t>(pEnd - pBegin), seed);
        }

    } // namespace


    GuiContex::Window::Window(PrimativeContext* pPrim, const char* pName) :
        pPrim(pPrim),
        name(pName),
        lastActiveFrame(0),
        active(false)
    {
        ID = idHash(name.begin(), name.end());
        // push window ID so id's will be unique per window.
        IDStack.push(ID);
    }

    GuiContex::ItemID GuiContex::Window::getID(const char* pLabel)
    {
        return getID(pLabel, pLabel + core::strUtil::strlen(pLabel));
    }

    GuiContex::ItemID GuiContex::Window::getID(const char* pBegin, const char* pEnd)
    {
        auto seed = IDStack.top();

        return idHash(pBegin, pBegin + safe_static_cast<size_t>(pEnd - pBegin), seed);
    }


    GuiContex::GuiContex() :
        pPrim_(nullptr),
        pCursor_(nullptr),
        pSpinner_(nullptr)
    {
        txtCtx_.pFont = nullptr;
        txtCtx_.col = Col_White;
        txtCtx_.size = Vec2f(24.f, 24.f);
        txtCtx_.effectId = 0;
        txtCtx_.flags.Set(font::DrawTextFlag::CENTER);
        txtCtx_.flags.Set(font::DrawTextFlag::CENTER_VER);

        mouseDown_.fill(false);

        activeId_ = INVALID_ITEM_ID;

        itemWidth_ = 400.f;
        itemWidthDefault_ = 256.f;

        style_.framePadding.Set(5.f, 8.f);
        style_.itemSpacing.Set(10.f, 8.f);

        style_.backgroundCol = Color8u(24, 24, 24, 255);
        style_.backgroundColFocus = Color8u(24, 24, 24, 255);
        style_.backgroundHoldCol = Color8u(30, 30, 30, 255);

        style_.borderCol = Color8u(25, 25, 25, 255);
        style_.borderColFocus = Col_Orange;

        style_.chkBoxCol = Color8u(255, 255, 255, 255);
        style_.chkBoxFillCol = Color8u(255, 255, 255, 255);

        style_.barBckCol = Color8u(36, 36, 36, 200);
        style_.barFillCol = Color8u(72, 72, 72, 200);

        currentFrame_ = 0;
    }

    GuiContex::~GuiContex()
    {
        for (auto* pWindow : windows_) {
            X_DELETE(pWindow, g_3dEngineArena);
        }
    }


    void GuiContex::init(engine::Material* pCursor, engine::Material* pSpinner)
    {
        pCursor_ = pCursor;
        pSpinner_ = pSpinner;
    }

    void GuiContex::setPrimContet(PrimativeContext* pPrim)
    {
        pPrim_ = pPrim;
    }

    void GuiContex::processInput(core::FrameInput& input)
    {
        cursorDelta_ = Vec2f::zero();

        for (auto& e : input.events)
        {
            if (e.deviceType == input::InputDeviceType::MOUSE)
            {
                if (e.keyId == input::KeyId::MOUSE_LEFT)
                {
                    mouseDown_[Mouse::LEFT] = e.action == input::InputState::PRESSED;
                }
                else if (e.keyId == input::KeyId::MOUSE_RIGHT)
                {
                    mouseDown_[Mouse::RIGHT] = e.action == input::InputState::PRESSED;
                }
                else if (e.keyId == input::KeyId::MOUSE_X)
                {
                    cursorDelta_.x += e.value;
                }
                else if (e.keyId == input::KeyId::MOUSE_Y)
                {
                    cursorDelta_.y += e.value;
                }
            }
        }

    }

    void GuiContex::beginFrame(Params& params)
    {
        params_ = params;
        hoveredId_ = INVALID_ITEM_ID;

        screenRect_ = params.rect;
        cursorPos_ = params.cursorPos;

        nextWindowData_.posVal = Vec2f(screenRect_.x1, screenRect_.y1);
        nextWindowData_.sizeVal = Vec2f(screenRect_.getWidth(), screenRect_.getHeight());

        // mark not visible
        for (auto* pWindow : windows_) {
            pWindow->active = false;
        }

        // clear current each frame.
        currentWindowStack_.clear();
        currentPopupStack_.clear();

        currentFrame_++;
    }

    void GuiContex::endFrame(void)
    {
        pPrim_->drawQuad(cursorPos_.x, cursorPos_.y, 32.f, 32.f, pCursor_, Col_White);

        while (!itemWidthStack_.isEmpty()) {
            popItemWidth();
        }
    }

    void GuiContex::begin(const char* pName, WindowFlags flags)
    {
        Window* pWindow = findWindow(pName);
        if (!pWindow) {
            pWindow = createWindow(pName, flags);
        }

        currentWindowStack_.push(pWindow);

        setCurrentWindow(pWindow);

        if (flags.IsSet(WindowFlag::Popup))
        {
            PopupRef& ref = openPopupStack_.top();
            ref.pWindow = pWindow;
            currentPopupStack_.push(ref);
        }

        bool justActive = pWindow->lastActiveFrame != currentFrame_;

        if (justActive)
        {
            pWindow->active = true;
            pWindow->lastActiveFrame = currentFrame_;
        }

        pWindow->pos = nextWindowData_.posVal;
        pWindow->size = nextWindowData_.sizeVal;

        pWindow->IDStack.clear();
        pWindow->IDStack.push(pWindow->ID);

        auto& dc = pWindow->dc;

        dc.indent.x = style_.windowPadding.x;
        dc.currentPosStart = pWindow->pos + Vec2f(dc.indent.x, style_.windowPadding.y);
        dc.currentPos = dc.currentPosStart;
        dc.lastItemID = INVALID_ITEM_ID;
        dc.lastItemRect = Rectf();
    }

    void GuiContex::end(void)
    {
        auto* pWindow = pCurrentWindow;

        currentWindowStack_.pop();

        if (pWindow->flags.IsSet(WindowFlag::Popup)) {
            currentPopupStack_.pop();
        }

        setCurrentWindow(currentWindowStack_.isEmpty() ? nullptr : currentWindowStack_.top());
    }

    void GuiContex::setFont(font::IFont* pFont)
    {
        txtCtx_.pFont = pFont;
    }

    void GuiContex::fill(Color8u col)
    {
        pPrim_->drawQuad(screenRect_, col);
    }

    void GuiContex::center(void)
    {
        auto* pWindow = pCurrentWindow;

        auto offset = itemWidth_ * 0.5f;

        pWindow->dc.currentPos = screenRect_.getCenter();
        pWindow->dc.currentPos.x -= offset;
    }

    void GuiContex::pushItemWidth(float width)
    {
        itemWidthStack_.push(itemWidth_);
        itemWidth_ = width;
    }

    void GuiContex::popItemWidth(void)
    {
        if (!itemWidthStack_.isEmpty()) {
            itemWidth_ = itemWidthStack_.top();
            itemWidthStack_.pop();
            return;
        }

        itemWidth_ = itemWidthDefault_;
    }

    bool GuiContex::button(const char* pText)
    {
        return button(pText, pText + core::strUtil::strlen(pText));
    }

    bool GuiContex::button(const char* pText, const char* pEnd)
    {
        auto* pWindow = pCurrentWindow;
        auto* pPrim = pWindow->pPrim;

        auto id = getID(pText, pEnd);
        auto labelSize = calcTextSize(pText, pEnd);

        // calculate pos / size
        auto pos = pWindow->dc.currentPos;
        auto size = calcItemSize(Vec2f::zero(), labelSize + style_.framePadding * 2.f);

        Rectf r(pos, pos + size);

        addItem(r, id);

        bool hovered, held;
        bool pressed = buttonBehavior(id, r, &hovered, &held);

        auto borderCol = style_.getBorderCol(hovered);
        auto bckCol = style_.getBackgroundCol(held, hovered);

        pPrim->drawQuad(r, bckCol);
        pPrim->drawRect(r, borderCol);
        pPrim->drawText(Vec3f(r.getCenter()), txtCtx_, pText);

        return pressed;
    }

    void GuiContex::label(const char* pText, const char* pEnd, Color8u col)
    {
        auto* pWindow = pCurrentWindow;
        auto* pPrim = pWindow->pPrim;

        auto id = getID(pText, pEnd);
        auto labelSize = calcTextSize(pText, pEnd);

        // calculate pos / size
        auto pos = pWindow->dc.currentPos;
        auto size = calcItemSize(Vec2f::zero(), labelSize + style_.framePadding * 2.f);

        Rectf r(pos, pos + size);

        addItem(r, id);

        auto oldCol = txtCtx_.col;
        txtCtx_.col = col;
        pPrim->drawText(Vec3f(r.getCenter()), txtCtx_, pText);
        txtCtx_.col = oldCol;
    }

    void GuiContex::text(const char* pText, const char* pEnd, Color8u col)
    {
        auto* pWindow = pCurrentWindow;
        auto* pPrim = pWindow->pPrim;

        auto oldCol = txtCtx_.col;
        txtCtx_.col = col;
        txtCtx_.flags.Remove(font::DrawTextFlag::CENTER);
        txtCtx_.size = Vec2f(36, 36);

        auto id = getID(pText, pEnd);
        auto labelSize = calcTextSize(pText, pEnd);

        auto pos = pWindow->dc.currentPos;
        auto size = calcItemSize(Vec2f::zero(), labelSize + style_.framePadding * 2.f);

        Rectf r(pos, pos + size);

        addItem(r, id);

        pPrim->drawText(Vec3f(pos), txtCtx_, pText, pEnd);

        txtCtx_.size = Vec2f(24.f, 24.f);
        txtCtx_.col = oldCol;
        txtCtx_.flags.Set(font::DrawTextFlag::CENTER);
    }

    void GuiContex::slider(const char* pLabel, const char* pVarName, float increment)
    {
        auto* pVar = gEnv->pConsole->getCVar(core::string_view(pVarName));
        if (!pVar) {
            X_ERROR("Gui", "Failed to find var for slider: \"%s\"", pVarName);
            return;
        }

        auto type = pVar->GetType();
        if (type != core::VarFlag::FLOAT) {
            return;
        }

        auto* pWindow = pCurrentWindow;
        auto* pPrim = pWindow->pPrim;

        float min = pVar->GetMin();
        float max = pVar->GetMax();
        float value = pVar->GetFloat();
        float range = max - min;
        float percent = (value - min) / range;

        auto id = getID(pLabel);

        // so i want to just draw like a box?
        auto width = itemWidth_;
        auto height = 32.f;
        auto pos = pWindow->dc.currentPos;
        auto size = Vec2f(width, height + style_.framePadding.y * 2.f);

        Rectf r(pos, pos + size);
        Rectf bar(r);
        bar.set(r.getX1() + (r.getWidth() * 0.5f), r.getY1(), r.getX2(), r.getY2());

        auto barWidth = bar.getWidth() * percent;
        Rectf barFill;
        barFill.set(bar.getX1(), bar.getY1(), bar.getX1() + barWidth, bar.getY2());

        addItem(r, id);

        bool hovered = itemHoverable(id, r);
        if (hovered)
        {
            if (mouseDown_[Mouse::LEFT])
            {
                if (bar.contains(cursorPos_))
                {
                    setActiveID(id);
                }
            }
        }

        if (id == activeId_)
        {
            if (mouseDown_[Mouse::LEFT])
            {
                auto offsetX = cursorPos_.x - bar.getX1();
                auto barRange = bar.getWidth();

                offsetX = core::Min(offsetX, barRange);
                offsetX = core::Max(offsetX, 0.f);

                auto newPercent = offsetX / barRange;
                auto newValue = range * newPercent;

                // interval
                newValue = static_cast<float>(static_cast<int>((newValue + (increment * 0.5f)) / increment) * increment);

                pVar->Set(newValue);
            }
            else
            {
                clearActiveID();
            }
        }

        auto borderCol = style_.getBorderCol(hovered);
        auto bckCol = style_.getBackgroundCol(false, hovered);

        pPrim->drawQuad(r, bckCol);
        pPrim->drawQuad(bar, style_.barBckCol);
        pPrim->drawQuad(barFill, style_.barFillCol);
        pPrim->drawRect(r, borderCol);

        txtCtx_.flags.Remove(font::DrawTextFlag::CENTER);
        pPrim->drawText(Vec3f(r.getX1() + style_.framePadding.x, r.getY1() + (r.getHeight() * 0.5f), 1.f), txtCtx_, pLabel);

        core::StackString<16, char> valueStr;
        valueStr.setFmt("%g", value);

        txtCtx_.flags.Set(font::DrawTextFlag::RIGHT);

        pPrim->drawText(Vec3f(r.getX2() - style_.framePadding.x, r.getY1() + (r.getHeight() * 0.5f), 1.f), txtCtx_, valueStr.begin(), valueStr.end());

        txtCtx_.flags.Remove(font::DrawTextFlag::RIGHT);
        txtCtx_.flags.Set(font::DrawTextFlag::CENTER);
    }

    void GuiContex::checkbox(const char* pLabel, const char* pVarName)
    {
        auto* pVar = gEnv->pConsole->getCVar(core::string_view(pVarName));
        if (!pVar) {
            X_ERROR("Gui", "Failed to find var for checkbox: \"%s\"", pVarName);
            return;
        }

        auto type = pVar->GetType();
        if (type != core::VarFlag::INT) {
            X_ERROR("Gui", "Var must be type int for checkbox: \"%s\"", pVarName);
            return;
        }

        auto min = pVar->GetMinInt();
        auto max = pVar->GetMaxInt();
        auto value = pVar->GetInteger();

        // able to toggle?
        if (min > 0 || max < 1) {
            X_ERROR("Gui", "Var not supported for checkbox: \"%s\"", pVarName);
            return;
        }

        auto* pWindow = pCurrentWindow;
        auto* pPrim = pWindow->pPrim;

        auto id = getID(pLabel);

        auto width = itemWidth_;
        auto height = 32.f;
        auto pos = pWindow->dc.currentPos;
        auto size = Vec2f(width, height + style_.framePadding.y * 2.f);

        Rectf r(pos, pos + size);

        addItem(r, id);

        bool hovered, held;
        bool pressed = buttonBehavior(id, r, &hovered, &held);

        if (pressed)
        {
            value = math<int>::clamp(value);
            value = !value;

            pVar->Set(value);
        }

        auto boxSize = 20.f;
        auto boxOffsetY = 4.f;
        auto boxFillSpacing = 2.f;

        if (held) {
            boxFillSpacing += 2.f;
        }

        Rectf boxRing(pos, pos + size);
        boxRing.x1 = r.x2 - (boxSize + style_.framePadding.x + boxOffsetY);
        boxRing.x2 = boxRing.x1 + boxSize;
        boxRing.y1 = r.y1 + ((size.y - boxSize) * 0.5f);
        boxRing.y2 = boxRing.y1 + boxSize;

        Rectf boxFill(boxRing);
        boxFill.x1 += boxFillSpacing;
        boxFill.y1 += boxFillSpacing;
        boxFill.x2 -= (boxFillSpacing + 1.F);
        boxFill.y2 -= (boxFillSpacing + 1.F);

        auto borderCol = style_.getBorderCol(hovered);
        auto bckCol = style_.getBackgroundCol(held, hovered);

        pPrim->drawQuad(r, bckCol);
        pPrim->drawRect(r, borderCol);
        pPrim->drawRect(boxRing, style_.chkBoxCol);
        if (value > 0 || held) {
            pPrim->drawQuad(boxFill, style_.chkBoxFillCol);
        }

        txtCtx_.flags.Remove(font::DrawTextFlag::CENTER);
        pPrim->drawText(Vec3f(r.getX1() + style_.framePadding.x, r.getY1() + (r.getHeight() * 0.5f), 1.f), txtCtx_, pLabel);
        txtCtx_.flags.Set(font::DrawTextFlag::CENTER);
    }

    bool GuiContex::combo(const char* pLabel, core::span<const char*> items, int32_t& currentIdx)
    {
        const char* pPreviewValue = nullptr;

        if (currentIdx >= 0) {
            pPreviewValue = items[currentIdx];
        }

        if (!comboBegin(pLabel, pPreviewValue)) {
            return false;
        }

        bool changed = false;

        for (int32_t i = 0; i < items.size(); i++)
        {
            pushID(i);

            const bool selected = (i == currentIdx);
            const char* pItemText = items[i];

            if (selectable(pItemText, selected))
            {
                changed = true;
                currentIdx = i;
            }

            popID();
        }

        comboEnd();
        return changed;
    }

    bool GuiContex::comboBegin(const char* pLabel, const char* pPreviewValue)
    {
        auto* pWindow = pCurrentWindow;
        auto* pPrim = pWindow->pPrim;

        auto* pLabelEnd = pLabel + core::strUtil::strlen(pLabel);
        auto id = getID(pLabel, pLabelEnd);
        auto labelSize = calcTextSize(pLabel, pLabelEnd);

        // calculate pos / size
        auto pos = pWindow->dc.currentPos;
        auto size = calcItemSize(Vec2f::zero(), labelSize + style_.framePadding * 2.f);

        const float arrowSize = size.y;

        Rectf r(pos, pos + size);

        addItem(r, id);

        bool hovered, held;
        bool pressed = buttonBehavior(id, r, &hovered, &held);

        auto borderCol = style_.getBorderCol(hovered);
        auto bckCol = style_.getBackgroundCol(held, hovered);

        pPrim->drawQuad(r, bckCol);
        pPrim->drawRect(r, borderCol);

        if (pPreviewValue) {
            pPrim->drawText(Vec3f(r.getCenter()), txtCtx_, pPreviewValue);
        }

        bool popupOpen = isPopupOpen(id);
        if (pressed && !popupOpen) {
            openPopUp(id);
        }

        if (!popupOpen) {
            return false;
        }

        // need to set the size.
        // width should match.

        setNextWindowPos(Vec2f(pos.x, pos.y + size.y));
        setNextWindowSize(Vec2f(size.x, 60.f));

        begin("Meow", WindowFlag::Popup);

        return true;
    }

    void GuiContex::comboEnd(void)
    {
        endPopUp();
    }

    void GuiContex::list(void)
    {
        // draw a predefined list?
        // where do we get the data from?
        // well for predefined lists could just query it.

        // i need the session.
        // which is in the game.

        Vec2f itemSize(itemWidth_,30);

        auto* pWindow = pCurrentWindow;
        auto* pPrim = pWindow->pPrim;

        auto id = getID("list");

        // calculate pos / size
        auto pos = pWindow->dc.currentPos;
        Vec2f size;
        size.x = itemSize.x + (style_.framePadding.x * 2.f);
        size.y = (net::MAX_PLAYERS * (itemSize.y + style_.itemSpacing.y)) + (style_.framePadding.x * 2.f);

        Rectf r(pos, pos + size);

        addItem(r, id);

        auto txtCtx = txtCtx_;
        txtCtx.flags.Remove(font::DrawTextFlag::CENTER);
        txtCtx.flags.Remove(font::DrawTextFlag::CENTER_VER);
        txtCtx.size = Vec2f(16, 16);

        auto* pLobby = params_.pSession->getLobby(net::LobbyType::Party);
        auto numUsers = pLobby->getNumUsers();

#if 1
        auto freeSlots = pLobby->getNumFreeUserSlots();
        auto hostIdx = pLobby->getHostPeerIdx();
        auto& params = pLobby->getMatchParams();

        net::ChatMsg msg;
        while (pLobby->tryPopChatMsg(msg))
        {
            core::DateTimeStamp::Description timeStr;

            X_LOG0("Chat", "%s: \"%s\"", msg.dateTimeStamp.toString(timeStr), msg.msg.c_str());
        }

        // who#s in my lobbyyyyy!!
        core::StackString512 txt;
        txt.setFmt("---- GameLobby(%" PRIuS "/%" PRIuS ") ----\n", numUsers, numUsers + freeSlots);

        for (int32_t i = 0; i < numUsers; i++)
        {
            net::UserInfo info;
            pLobby->getUserInfoForIdx(i, info);

            bool isHost = (hostIdx == info.peerIdx);

            txt.appendFmt("\n%s ^8%.*s ^7peerIdx: ^8%" PRIi32 "^7", isHost ? "H" : "P", info.name.length(), info.name.data(), info.peerIdx);
        }

        pPrim->drawQuad(800.f, 200.f, 320.f + 320.f, 200.f, Color8u(40, 40, 40, 100));
        pPrim->drawText(Vec3f(802.f, 202.f, 1.f), txtCtx, txt.begin(), txt.end());

        txt.setFmt("Options:\nSlots: %" PRIi32 "\nMap: \"%s\"", params.numSlots, params.mapName.c_str());

        pPrim->drawText(Vec3f(1240.f, 202.f, 1.f), txtCtx, txt.begin(), txt.end());

#else

        // draw all the slots.
        pPrim->drawQuad(r, Col_Aquamarine);

        std::array < Rectf, net::MAX_PLAYERS> itemRects;

        for (int32_t i = 0; i<net::MAX_PLAYERS; i++)
        {
            Rectf& itemR = itemRects[i];
            itemR.x1 = r.x1 + style_.framePadding.x;
            itemR.x2 = itemR.x1 + itemSize.x;
            itemR.y1 = r.y1 + style_.framePadding.y + (i * (itemSize.y + style_.itemSpacing.y));
            itemR.y2 = itemR.y1 + itemSize.y;

            pPrim->drawQuad(itemR, style_.backgroundCol);
        }

        for (int32_t i = 0; i<num; i++)
        {
            net::UserInfo info;
            pParty->getUserInfoForIdx(i, info);
            
            Rectf& itemR = itemRects[i];

            pPrim->drawText(Vec3f(itemR.getUpperLeft()), txtCtx, info.pName);
        }
#endif
    }

    bool GuiContex::selectable(const char* pLabel, bool selected)
    {
        X_UNUSED(pLabel, selected);

        auto* pWindow = pCurrentWindow;

        auto id = pWindow->getID(pLabel);

        id = 0;

        return false;
    }

    void GuiContex::pacifier(float dt)
    {
        // what to draw!
        // the options are endless..
        const float size = 64.f;
        const float radius = size * 0.5f;
        const float padding = 16.f;
        const float offset = radius + padding;

        // i want it to spin!
        static float elapse = 0.f;

        elapse += dt;

        auto rotation = elapse / 5.f;

        auto mat = Matrix22f::createRotation(::toRadians(-rotation));

        Vec2f tl(-radius, -radius);
        Vec2f bl(-radius, radius);
        Vec2f tr(radius, -radius);
        Vec2f br(radius, radius);

        Vec2f base(screenRect_.x2 - offset, screenRect_.y2 - offset);
        tl = (mat * tl) + base;
        tr = (mat * tr) + base;
        bl = (mat * bl) + base;
        br = (mat * br) + base;

        auto* pPrim = pCurrentWindow->pPrim;
        pPrim->drawQuad(
            Vec3f(tl),
            Vec3f(tr),
            Vec3f(bl),
            Vec3f(br),
            pSpinner_,
            Color8u(255, 255, 255, 255)
        );
    }


    Vec2f GuiContex::calcTextSize(const char* pBegin, const char* pEnd)
    {
        return txtCtx_.pFont->GetTextSize(pBegin, pEnd, txtCtx_);
    }

    Vec2f GuiContex::calcItemSize(Vec2f size, Vec2f defaultSize)
    {
        return calcItemSize(size, defaultSize.x, defaultSize.y);
    }


    Vec2f GuiContex::calcItemSize(Vec2f size, float defaultX, float defaultY)
    {
        X_UNUSED(defaultX);

        if (size.x <= 0.f) {
            size.x = itemWidth_;
        }

        if (size.y <= 0.f) {
            size.y = defaultY;
        }

        return size;
    }

    void GuiContex::addItem(const Rectf& r, ItemID id)
    {
        auto* pWindow = pCurrentWindow;
        auto& dc = pWindow->dc;

        dc.currentPos.y += r.getHeight() + style_.itemSpacing.y;
        dc.lastItemID = id;
        dc.lastItemRect = r;
    }

    bool GuiContex::itemHoverable(ItemID id, const Rectf& r)
    {
        if (hoveredId_ != INVALID_ITEM_ID && hoveredId_ != id) {
            return false;
        }
        if (activeId_ != INVALID_ITEM_ID && activeId_ != id) {
            return false;
        }

        if (!r.contains(cursorPos_)) {
            return false;
        }

        hoveredId_ = id;
        return true;
    }

    bool GuiContex::buttonBehavior(ItemID id, const Rectf& r, bool* pHovered, bool* pHeld)
    {
        bool hovered = itemHoverable(id, r);

        // Mouse
        if (hovered)
        {
            if (mouseDown_[Mouse::LEFT])
            {
                if (r.contains(cursorPos_))
                {
                    setActiveID(id);
                }
            }
        }

        bool pressed = false;
        bool held = false;

        if (id == activeId_)
        {
            if (!mouseDown_[Mouse::LEFT])
            {
                if (hovered)
                {
                    pressed = true;
                }

                clearActiveID();
            }
            else
            {
                held = true;
            }
        }

        if (pHovered) {
            *pHovered = hovered;
        }

        if (pHeld) {
            *pHeld = held;
        }

        return pressed;
    }

    bool GuiContex::isPopupOpen(ItemID id) const
    {
        return !openPopupStack_.isEmpty() && openPopupStack_.top().popupId == id;
    }

    void GuiContex::openPopUp(ItemID id)
    {
        PopupRef popup;
        popup.popupId = id;
        //   popup.OpenParentId = parent_window->IDStack.back();
        //   popup.OpenMousePos = g.IO.MousePos;
        //   popup.OpenPopupPos = NavCalcPreferredRefPos();

        openPopupStack_.push(popup);
    }

    void GuiContex::endPopUp(void)
    {
        end();
    }

    GuiContex::Window* GuiContex::findWindow(const char* pName) const
    {
        auto id = idHash(pName);

        for (auto* pWindow : windows_) {
            if (pWindow->ID == id) {
                return pWindow;
            }
        }

        return nullptr;
    }

    GuiContex::Window* GuiContex::createWindow(const char* pName, WindowFlags flags)
    {
        Window* pWindow = X_NEW(Window, g_3dEngineArena, "UIWindow")(pPrim_, pName);

        pWindow->flags = flags;

        windows_.push_back(pWindow);

        return pWindow;
    }

    void GuiContex::setCurrentWindow(Window* pWindow)
    {
        pCurrentWindow = pWindow;
    }

    void GuiContex::setActiveID(ItemID id)
    {
        activeId_ = id;
    }

    void GuiContex::pushID(ItemID id)
    {
        pCurrentWindow->IDStack.push(id);
    }

    void GuiContex::popID(void)
    {
        pCurrentWindow->IDStack.pop();
    }

    void GuiContex::clearActiveID(void)
    {
        activeId_ = INVALID_ITEM_ID;
    }

    GuiContex::ItemID GuiContex::getID(const char* pLabel) const
    {
        return pCurrentWindow->getID(pLabel);
    }

    GuiContex::ItemID GuiContex::getID(const char* pBegin, const char* pEnd) const
    {
        return pCurrentWindow->getID(pBegin, pEnd);
    }




} // namespace gui

X_NAMESPACE_END
#include "stdafx.h"
#include "GuiContex.h"

#include <IConsole.h>
#include <IInput.h>
#include <IFrameData.h>

#include "Drawing\PrimativeContext.h"

X_NAMESPACE_BEGIN(engine)

namespace gui
{

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

        style_.btnCol = Color8u(24, 24, 24, 255);
        style_.btnHover = Color8u(70, 70, 70, 255);
        style_.btnHold = Color8u(60, 20, 20, 255);

        style_.borderCol = Color8u(25, 25, 25, 255);
        style_.borderColForcus = Col_Orange;

        style_.chkBoxCol = Color8u(255, 255, 255, 255);
        style_.chkBoxFillCol = Color8u(255, 255, 255, 255);
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

    void GuiContex::begin(Params& params)
    {
        hoveredId_ = INVALID_ITEM_ID;

        rect_ = params.rect;
        cursorPos_ = params.cursorPos;


        dc_.currentPos = Vec2f((rect_.getWidth() * 0.1f), 400.f);
        dc_.lastItemID = INVALID_ITEM_ID;
        dc_.lastItemRect = Rectf();
    }

    void GuiContex::end(void)
    {
        pPrim_->drawQuad(cursorPos_.x, cursorPos_.y, 32.f, 32.f, pCursor_, Col_White);

        while (!itemWidthStack_.isEmpty()) {
            popItemWidth();
        }
    }


    void GuiContex::setFont(font::IFont* pFont)
    {
        txtCtx_.pFont = pFont;
    }

    void GuiContex::fill(Color8u col)
    {
        pPrim_->drawQuad(rect_, col);
    }

    void GuiContex::center(void)
    {
        auto center = rect_.getWidth() * 0.5f;
        auto offset = itemWidth_ * 0.5f;

        auto x = center - offset;

        dc_.currentPos.x = x;
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
        auto id = getID(pText, pEnd);
        auto labelSize = calcTextSize(pText, pEnd);

        // calculate pos / size
        auto pos = dc_.currentPos;
        auto size = calcItemSize(Vec2f::zero(), labelSize + style_.framePadding * 2.f);

        Rectf r(pos, pos + size);

        addItem(r, id);

        bool hovered = itemHoverable(id, r);
        if (hovered)
        {
            if (mouseDown_[Mouse::LEFT])
            {
                setActiveID(id);
            }
        }

        bool held = false;
        bool pressed = false;

        if (id == activeId_)
        {
            if (mouseDown_[Mouse::LEFT])
            {
                held = true;
            }
            else
            {
                if (hovered) {
                    pressed = true;
                }
                clearActiveID();
            }
        }

        auto borderCol = hovered ? style_.borderColForcus : style_.borderCol;
        auto btnCol = style_.btnCol;

        if (held) {
            btnCol = style_.btnHold;
        }
        else if (hovered) {
            btnCol = style_.btnHover;
        }

        pPrim_->drawQuad(r, btnCol);
        pPrim_->drawRect(r, borderCol);
        pPrim_->drawText(Vec3f(r.getCenter()), txtCtx_, pText);

        return pressed;
    }

    void GuiContex::text(const char* pText, const char* pEnd, Color8u col)
    {
        auto oldCol = txtCtx_.col;
        txtCtx_.col = col;
        txtCtx_.flags.Remove(font::DrawTextFlag::CENTER);
        txtCtx_.size = Vec2f(36, 36);

        auto id = getID(pText, pEnd);
        auto labelSize = calcTextSize(pText, pEnd);

        auto pos = dc_.currentPos;
        auto size = calcItemSize(Vec2f::zero(), labelSize + style_.framePadding * 2.f);

        Rectf r(pos, pos + size);

        addItem(r, id);

        pPrim_->drawText(Vec3f(pos), txtCtx_, pText, pEnd);

        txtCtx_.size = Vec2f(24.f, 24.f);
        txtCtx_.col = oldCol;
        txtCtx_.flags.Set(font::DrawTextFlag::CENTER);
    }

    void GuiContex::slider(const char* pLabel, const char* pVarName, float increment)
    {
        auto* pVar = gEnv->pConsole->GetCVar(pVarName);
        if (!pVar) {
            X_ERROR("Gui", "Failed to find var for slider: \"%s\"", pVarName);
            return;
        }

        auto type = pVar->GetType();
        if (type != core::VarFlag::FLOAT) {
            return;
        }

        float min = pVar->GetMin();
        float max = pVar->GetMax();
        float value = pVar->GetFloat();
        float range = max - min;
        float percent = (value - min) / range;

        auto id = getID(pLabel);

        // so i want to just draw like a box?
        auto width = itemWidth_;
        auto height = 32.f;
        auto pos = dc_.currentPos;
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

        auto borderCol = hovered ? style_.borderColForcus : style_.borderCol;


        Color8u bckCol(24, 24, 24, 200);
        Color8u barBckCol(36, 36, 36, 200);
        Color8u barFillCol(72, 72, 72, 200);

        bckCol = style_.btnCol;

        pPrim_->drawQuad(r, bckCol);
        pPrim_->drawQuad(bar, barBckCol);
        pPrim_->drawQuad(barFill, barFillCol);
        pPrim_->drawRect(r, borderCol);

        txtCtx_.flags.Remove(font::DrawTextFlag::CENTER);
        pPrim_->drawText(Vec3f(r.getX1() + style_.framePadding.x, r.getY1() + (r.getHeight() * 0.5f), 1.f), txtCtx_, pLabel);

        core::StackString<16, char> valueStr;
        valueStr.setFmt("%g", value);

        txtCtx_.flags.Set(font::DrawTextFlag::RIGHT);
        
        pPrim_->drawText(Vec3f(r.getX2() - style_.framePadding.x, r.getY1() + (r.getHeight() * 0.5f), 1.f), txtCtx_, valueStr.begin(), valueStr.end());

        txtCtx_.flags.Remove(font::DrawTextFlag::RIGHT);
        txtCtx_.flags.Set(font::DrawTextFlag::CENTER);
    }

    void GuiContex::checkbox(const char* pLabel, const char* pVarName)
    {
        auto* pVar = gEnv->pConsole->GetCVar(pVarName);
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

        auto id = getID(pLabel);

        auto width = itemWidth_;
        auto height = 32.f;
        auto pos = dc_.currentPos;
        auto size = Vec2f(width, height + style_.framePadding.y * 2.f);

        Rectf r(pos, pos + size);

        addItem(r, id);

        bool hovered = itemHoverable(id, r);
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

        bool held = false;

        if (id == activeId_)
        {
            if (!mouseDown_[Mouse::LEFT])
            {
                // toggle.
                if (hovered)
                {
                    value = math<int>::clamp(value);
                    value = !value;

                    pVar->Set(value);
                }

                clearActiveID();
            }
            else
            {
                held = true;
            }
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

        auto borderCol = hovered ? style_.borderColForcus : style_.borderCol;

        pPrim_->drawQuad(r, style_.btnCol);
        pPrim_->drawRect(r, borderCol);
        pPrim_->drawRect(boxRing, style_.chkBoxCol);
        if (value > 0 || held) {
            pPrim_->drawQuad(boxFill, style_.chkBoxFillCol);
        }
        
        txtCtx_.flags.Remove(font::DrawTextFlag::CENTER);
        pPrim_->drawText(Vec3f(r.getX1() + style_.framePadding.x, r.getY1() + (r.getHeight() * 0.5f), 1.f), txtCtx_, pLabel);
        txtCtx_.flags.Set(font::DrawTextFlag::CENTER);
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

        Vec2f base(rect_.x2 - offset, rect_.y2 - offset);
        tl = (mat * tl) + base;
        tr = (mat * tr) + base;
        bl = (mat * bl) + base;
        br = (mat * br) + base;

        pPrim_->drawQuad(
            Vec3f(tl),
            Vec3f(tr),
            Vec3f(bl),
            Vec3f(br),
            pSpinner_,
            Color8u(255,255,255,255)
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
        dc_.currentPos.y += r.getHeight() + style_.itemSpacing.y;
        dc_.lastItemID = id;
        dc_.lastItemRect = r;
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


    void GuiContex::setActiveID(ItemID id)
    {
        activeId_ = id;
    }

    void GuiContex::clearActiveID(void)
    {
        activeId_ = INVALID_ITEM_ID;
    }

    GuiContex::ItemID GuiContex::getID(const char* pLabel)
    {
        return core::Hash::Fnv1Hash(pLabel, core::strUtil::strlen(pLabel));
    }

    GuiContex::ItemID GuiContex::getID(const char* pBegin, const char* pEnd)
    {
        return core::Hash::Fnv1Hash(pBegin, safe_static_cast<size_t>(pEnd - pBegin));
    }




} // namespace gui

X_NAMESPACE_END
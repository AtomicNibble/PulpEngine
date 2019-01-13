#include "stdafx.h"
#include "Menu.h"
#include "MenuManger.h"

#include <IFrameData.h>

X_NAMESPACE_BEGIN(engine)


namespace gui
{

    Menu::Menu(core::string_view name) :
        core::AssetBase(name, assetDb::AssetType::MENU),
        pScriptTable_(nullptr)
    {
    }

    Menu::~Menu()
    {
    }

    void Menu::draw(core::FrameData& frame)
    {
        if (!ensureScript()) {
            return;
        }

        if (updateFunc_.getType() != script::Type::Function) {
            X_ERROR("Menu", "Update function is invalid for: \"%s\"", name_.c_str());
            return;
        }

        auto uiDelta = frame.timeInfo.deltas[core::ITimer::Timer::UI].GetMilliSeconds();

        gEnv->pScriptSys->call(updateFunc_.pFunction_, script::ScriptValue(uiDelta));
    }

    void Menu::onOpen(void)
    {
        if (!ensureScript()) {
            return;
        }

        // have a onOpen?
        if (onOpenFunc_.getType() != script::Type::Function) {
            return;
        }

        gEnv->pScriptSys->call(onOpenFunc_.pFunction_);
    }

    bool Menu::ensureScript(void)
    {
        if (pScriptTable_) {
            return true;
        }

        auto* pScriptSys = gEnv->pScriptSys;

        const char* pBegin = data_.ptr();
        const char* pEnd = pBegin + dataSize_;

        pScriptTable_ = pScriptSys->createTable(true);

        if (!pScriptSys->loadBufferToTable(pBegin, pEnd, name_.c_str(), pScriptTable_)) {
            return false;
        }

        pScriptTable_->getValueAny("Update", updateFunc_);
        pScriptTable_->getValueAny("OnOpen", onOpenFunc_);

        if (updateFunc_.getType() != script::Type::Function) {
            return false;
        }

        return true;
    }

    bool Menu::processData(core::UniquePointer<char[]> data, uint32_t dataSize)
    {
        data_ = std::move(data);
        dataSize_ = dataSize;

        // need to remake the script table.
        if (pScriptTable_)
        {
            updateFunc_.clear();
            onOpenFunc_.clear();

            pScriptTable_->release();
            pScriptTable_ = nullptr;
        }

        return true;
    }
  

} // namespace gui

X_NAMESPACE_END
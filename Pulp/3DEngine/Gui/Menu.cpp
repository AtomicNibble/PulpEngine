#include "stdafx.h"
#include "Menu.h"
#include "MenuManger.h"


X_NAMESPACE_BEGIN(engine)


namespace gui
{

    Menu::Menu(core::string& name) :
        core::AssetBase(name, assetDb::AssetType::MENU),
        pScriptTable_(nullptr)
    {
    }

    Menu::~Menu()
    {
    }

    void Menu::draw(void)
    {
        auto* pScriptSys = gEnv->pScriptSys;

        if (!pScriptTable_) {
            
            const char* pBegin = data_.ptr();
            const char* pEnd = pBegin + dataSize_;

            pScriptTable_ = pScriptSys->createTable(true);

            if (!pScriptSys->loadBufferToTable(pBegin, pEnd, name_.c_str(), pScriptTable_)) {
                return;
            }

            pScriptTable_->getValueAny("Update", updateFunc_);

            if (updateFunc_.getType() != script::Type::Function) {
                return;
            }
        }

        pScriptSys->call(updateFunc_.pFunction_);
    }

    bool Menu::processData(core::UniquePointer<char[]> data, uint32_t dataSize)
    {
        data_ = std::move(data);
        dataSize_ = dataSize;

        // need to remake the script table.
        if (pScriptTable_)
        {
            updateFunc_.clear();

            pScriptTable_->release();
            pScriptTable_ = nullptr;
        }

        return true;
    }
  

} // namespace gui

X_NAMESPACE_END
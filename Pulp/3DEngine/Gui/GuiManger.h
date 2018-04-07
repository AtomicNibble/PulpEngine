#pragma once

#include <IGui.h>
#include <IInput.h>

#include "Gui.h"

X_NAMESPACE_DECLARE(core,
                    struct IConsoleCmdArgs);

X_NAMESPACE_BEGIN(engine)

namespace gui
{
    class XGuiManager : public IGuiManger
        , public core::IXHotReload
        , public input::IInputEventListner
    {
        typedef core::Array<XGui*> Guis;

    public:
        XGuiManager(core::MemoryArenaBase* arena, XMaterialManager* pMatMan);
        ~XGuiManager() X_FINAL;

        //IGuiManger
        bool init(void) X_FINAL;
        void shutdown(void) X_FINAL;

        IGui* loadGui(const char* name) X_FINAL;
        IGui* findGui(const char* name) X_FINAL;

        void listGuis(const char* wildcardSearch = nullptr) const X_FINAL;
        //~IGuiManger

        // IXHotReload
        void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_FINAL;
        // ~IXHotReload

        // IInputEventListner
        bool OnInputEvent(const input::InputEvent& event) X_FINAL;
        bool OnInputEventChar(const input::InputEvent& event) X_FINAL;
        // ~IInputEventListner

        X_INLINE bool ShowDeubug(void) const;
        X_INLINE engine::Material* GetCursor(void) const;

    private:
        void Command_ListUis(core::IConsoleCmdArgs* pArgs);

    private:
        core::MemoryArenaBase* arena_;
        XMaterialManager* pMatMan_;

        Rectf screenRect_;
        Guis guis_;

        int var_showDebug_;

        engine::Material* pCursorArrow_;
    };

    X_INLINE bool XGuiManager::ShowDeubug(void) const
    {
        return var_showDebug_ == 1;
    }

    X_INLINE engine::Material* XGuiManager::GetCursor(void) const
    {
        return pCursorArrow_;
    }

} // namespace gui

X_NAMESPACE_END

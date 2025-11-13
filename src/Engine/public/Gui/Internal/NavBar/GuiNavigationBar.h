#pragma once
#include <string>
#include <string_view>
#include "Gui/Internal/GuiLayoutManager.h"

struct ImVec2;

namespace BixEngine::Gui
{
    class GuiSystem;
    class GuiPanel;
    class GuiManager;
}

namespace BixEngine::Core
{
    class GuiModule;


    class GuiNavigationBar
    {
    public:
        GuiNavigationBar(Gui::GuiSystem& guiSystem, Gui::GuiLayoutManager& layoutManager, GuiModule& owner);
        ~GuiNavigationBar() = default;

        void Render();

    private:
        bool DrawNavigationButton(const std::string& label, bool isActive, float buttonHeight) const;
        bool DrawCloseButton(std::string_view label, float buttonHeight) const;

        void DrawSceneButton(float buttonHeight);
        void DrawAssetEditorTabs(float buttonHeight);

        Gui::GuiSystem* guiSystem_{nullptr};
        Gui::GuiLayoutManager* layoutManager_{nullptr};
        GuiModule* owner_{nullptr};
    };
}

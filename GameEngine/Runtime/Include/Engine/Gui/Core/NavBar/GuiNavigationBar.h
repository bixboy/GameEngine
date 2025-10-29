#pragma once

#include <string>
#include <string_view>

#include "Engine/Gui/Core/GuiLayoutManager.h"

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

    /**
     * @brief Barre de navigation supérieure de l’éditeur.
     */
    class GuiNavigationBar
    {
    public:
        GuiNavigationBar(Gui::GuiSystem& guiSystem, Gui::GuiLayoutManager& layoutManager, GuiModule& owner);
        ~GuiNavigationBar() = default;

        /** Affiche la barre de navigation principale. */
        void Render();

    private:
        
        bool DrawNavigationButton(const std::string& label, bool isActive, float buttonHeight) const;
        bool DrawCloseButton(std::string_view label, float buttonHeight) const;

        void DrawSceneButton(float buttonHeight);
        void DrawActorEditorTabs(float buttonHeight);

    private:
        Gui::GuiSystem* guiSystem_{nullptr};
        Gui::GuiLayoutManager* layoutManager_{nullptr};
        GuiModule* owner_{nullptr};
    };
}

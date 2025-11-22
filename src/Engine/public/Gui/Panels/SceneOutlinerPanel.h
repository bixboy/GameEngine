#pragma once

#include <array>
#include <functional>

#include "Gui/DefaultEngineGui.h"
#include "Gui/GuiPanelBase.h"

namespace BixEngine::Game
{
    class Actor;
    class SceneManager;
}

namespace BixEngine::Gui
{
    class SceneOutlinerPanel : public GuiPanelBase
    {
    public:
        SceneOutlinerPanel(std::function<Game::SceneManager*()> sceneProvider, std::function<Game::Actor*()> selectionGetter, std::function<void(Game::Actor*)> selectionSetter);
        
        explicit SceneOutlinerPanel(const DefaultEngineGuiContext& context);

        void Draw() override;

    private:
        std::function<Game::SceneManager*()> sceneManagerProvider_{};
        std::function<Game::Actor*()> selectedActorGetter_{};
        std::function<void(Game::Actor*)> selectedActorSetter_{};
        std::array<char, 128> searchBuffer_{};
        
        // Rename state
        std::array<char, 128> renameBuffer_{};
        Game::Actor* actorToRename_{nullptr};
        bool openRenamePopup_{false};
        Game::Actor* actorWithContextMenu_{nullptr};
    };
}

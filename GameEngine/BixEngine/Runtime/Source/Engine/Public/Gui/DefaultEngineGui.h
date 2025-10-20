#pragma once

#include <functional>

namespace BixEngine
{
    namespace Core { class Timer; }
    namespace Game { class SceneManager; class Actor; }
}

namespace BixEngine::Gui
{
    class GuiManager;
    class GuiPanel;

    struct DefaultEngineGuiContext
    {
        Core::Timer* timer{nullptr};
        std::function<Game::SceneManager*()> sceneManagerProvider{};
        const float* lastDeltaTime{nullptr};
        std::function<Game::Actor*()> selectedActorGetter{};
        std::function<void(Game::Actor*)> selectedActorSetter{};
    };

    struct DefaultEngineGuiPanels
    {
        GuiPanel* statsPanel{nullptr};
        GuiPanel* sceneOutlinerPanel{nullptr};
        GuiPanel* contentBrowserPanel{nullptr};
        GuiPanel* actorInspectorPanel{nullptr};
    };

    DefaultEngineGuiPanels CreateDefaultEngineGui(GuiManager& guiManager, const DefaultEngineGuiContext& context);
}

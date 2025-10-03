#pragma once

#include <functional>

namespace Engine
{
    namespace Core { class Timer; }
    namespace Game { class SceneManager; }
}

namespace Engine::Gui
{
    class GuiManager;
    class GuiPanel;

    struct DefaultEngineGuiContext
    {
        Core::Timer* timer{nullptr};
        std::function<Game::SceneManager*()> sceneManagerProvider{};
        const float* lastDeltaTime{nullptr};
    };

    struct DefaultEngineGuiPanels
    {
        GuiPanel* statsPanel{nullptr};
        GuiPanel* sceneOutlinerPanel{nullptr};
        GuiPanel* contentBrowserPanel{nullptr};
    };

    DefaultEngineGuiPanels CreateDefaultEngineGui(GuiManager& guiManager, const DefaultEngineGuiContext& context);
}

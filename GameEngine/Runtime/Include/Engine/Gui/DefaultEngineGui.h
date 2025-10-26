#pragma once

#include <functional>
#include <utility>
#include <filesystem>
#include <vector>

namespace BixEngine
{
    namespace Core { class Timer; }
    namespace Game { class SceneManager; class Actor; }
}

struct SDL_Texture;

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
        std::function<SDL_Texture*()> sceneRenderTextureProvider{};
        std::function<std::pair<int, int>()> sceneRenderTextureSizeProvider{};
        std::function<void(const std::vector<std::filesystem::path>&)> openScriptFilesInEditor{};
    };

    struct DefaultEngineGuiPanels
    {
        GuiPanel* sceneViewportPanel{nullptr};
        GuiPanel* statsPanel{nullptr};
        GuiPanel* sceneOutlinerPanel{nullptr};
        GuiPanel* contentBrowserPanel{nullptr};
        GuiPanel* actorInspectorPanel{nullptr};
    };

    DefaultEngineGuiPanels CreateDefaultEngineGui(GuiManager& guiManager, const DefaultEngineGuiContext& context);
}

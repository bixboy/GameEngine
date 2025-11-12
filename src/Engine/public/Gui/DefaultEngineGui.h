#pragma once

#include <functional>
#include <filesystem>
#include <utility>
#include <vector>
#include <unordered_map>
#include <string>
#include "Gui/Internal/GuiPanel.h"

namespace BixEngine
{
    namespace Core
    {
        class Timer;
    }

    namespace Game
    {
        class SceneManager;
        class Actor;
    }

    namespace Input
    {
        struct MouseStatistics;
    }
}

struct SDL_Texture;

namespace BixEngine::Gui
{
    class GuiManager;

    // ────────────────────────────────────────────────
    // ⚙️ Contexte partagé entre tous les panneaux
    // ────────────────────────────────────────────────

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
        std::function<void(const std::filesystem::path&)> openAssetInEditor{};
        std::function<const Input::MouseStatistics*()> mouseStatsProvider{};
        bool bEnableGui{true};
    };

    // ────────────────────────────────────────────────
    // 📋 Ensemble des panneaux créés
    // ────────────────────────────────────────────────

    struct DefaultEngineGuiPanels
    {
        std::unordered_map<std::string, GuiPanel*> allPanels;
        GuiPanel* sceneViewportPanel{nullptr};
        GuiPanel* statsPanel{nullptr};
        GuiPanel* sceneOutlinerPanel{nullptr};
        GuiPanel* contentBrowserPanel{nullptr};
        GuiPanel* actorInspectorPanel{nullptr};

        bool Has(const std::string& name) const
        {
            return allPanels.contains(name);
        }
    };

    // ────────────────────────────────────────────────
    // 🧩 Fonction principale de création des panneaux
    // ────────────────────────────────────────────────

    DefaultEngineGuiPanels CreateDefaultEngineGui(GuiManager& guiManager, const DefaultEngineGuiContext& context);
}

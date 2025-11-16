#pragma once

#include <functional>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <string>

#include "Input.h"
#include "SceneManager.h"
#include "Timer.h"
#include "Gui/Panels/GuiPanel.h"


namespace BixEngine::Gui
{
    class GuiManager;

    // ==========================================================================
    //  Contexte global partagé par tous les panneaux
    // ==========================================================================

    struct DefaultEngineGuiContext
    {
        Core::Timer* timer{nullptr};
        std::function<Game::SceneManager*()> sceneManagerProvider{};
        const float* lastDeltaTime{nullptr};
        std::function<Game::Actor*()> selectedActorGetter{};
        std::function<void(Game::Actor*)> selectedActorSetter{};
        std::function<SDL_Texture*()> sceneRenderTextureProvider{};
        std::function<std::pair<int,int>()> sceneRenderTextureSizeProvider{};
        std::function<void(const std::vector<std::filesystem::path>&)> openScriptFilesInEditor{};
        std::function<void(const std::filesystem::path&)> openAssetInEditor{};
        std::function<const Input::MouseStatistics*()> mouseStatsProvider{};
        bool bEnableGui{true};
    };

    // ==========================================================================
    //  Tous les panneaux créés
    // ==========================================================================

    struct DefaultEngineGuiPanels
    {
        std::unordered_map<std::string, GuiPanel*> allPanels;

        GuiPanel* sceneViewportPanel{nullptr};
        GuiPanel* statsPanel{nullptr};
        GuiPanel* sceneOutlinerPanel{nullptr};
        GuiPanel* contentBrowserPanel{nullptr};
        GuiPanel* actorInspectorPanel{nullptr};

        bool Has(const std::string& name) const { return allPanels.contains(name); }
    };

    class DefaultEngineGuiPanelManager
    {
    public:
        DefaultEngineGuiPanelManager(GuiManager& guiManager, const DefaultEngineGuiContext& context);

        /** Crée tous les panneaux, les enregistre, les retourne. */
        DefaultEngineGuiPanels CreatePanels();

    private:
        template <typename PanelT, typename... Args>
        GuiPanel* TryRegisterPanel(const char* name, GuiManager::PanelDescriptor descriptor, Args&&... args);

        GuiManager& guiManager_;
        const DefaultEngineGuiContext& context_;
    };

    DefaultEngineGuiPanels CreateDefaultEngineGui(GuiManager& guiManager, const DefaultEngineGuiContext& context);
}

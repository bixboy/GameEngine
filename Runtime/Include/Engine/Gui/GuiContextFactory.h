#pragma once

#include <filesystem>
#include <functional>
#include <vector>

namespace BixEngine
{
    namespace Core { class Timer; class SubsystemManager; }
    namespace Game { class Actor; class SceneManager; }
}

struct SDL_Texture;

namespace BixEngine::Gui
{
    struct DefaultEngineGuiContext;

    // ────────────────────────────────────────────────
    // ⚙️ Paramètres de création du contexte GUI
    // ────────────────────────────────────────────────
    struct DefaultEngineGuiContextArgs
    {

        // Accès global aux sous-systèmes
        Core::SubsystemManager& subsystems;

        // Durée du dernier frame
        const float* lastDeltaTime{nullptr};

        // Pointeur vers l’acteur sélectionné
        Game::Actor** selectedActorSlot{nullptr};

        // Texture du rendu 3D (viewport)
        SDL_Texture** sceneViewportTexture{nullptr};

        // Fournisseur de taille du viewport
        std::function<std::pair<int, int>()> sceneViewportSizeProvider{};
        
        std::function<void(const std::vector<std::filesystem::path>&)> openScriptFilesInEditor{};
        std::function<void(const std::filesystem::path&)> openAssetInEditor{};

        // Nouveau : permet un mode headless
        bool bEnableGui{true};
    };

    // ────────────────────────────────────────────────
    // 🧩 Factory
    // ────────────────────────────────────────────────
    class DefaultEngineGuiContextFactory
    {
    public:
        explicit DefaultEngineGuiContextFactory(Core::SubsystemManager& subsystems);

        [[nodiscard]] DefaultEngineGuiContext CreateContext(const DefaultEngineGuiContextArgs& args) const;

    private:
        Core::SubsystemManager& subsystems_;

        static std::function<std::pair<int, int>()> DefaultSizeProvider();
        static std::function<void(const std::vector<std::filesystem::path>&)> DefaultScriptOpener();
        static std::function<void(const std::filesystem::path&)> DefaultAssetOpener();
    };
}

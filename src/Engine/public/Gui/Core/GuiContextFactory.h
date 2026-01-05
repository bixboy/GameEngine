#pragma once
#include <filesystem>
#include <functional>
#include <vector>
#include <utility>
#include "Systems/Core/SubsystemManager.h"

struct SDL_Texture;


namespace BixEngine::Gui
{
    struct DefaultEngineGuiContext;
    
    struct DefaultEngineGuiContextArgs
    {
        const float* lastDeltaTime{nullptr};

        Game::Actor** selectedActorSlot{nullptr};

        SDL_Texture** sceneViewportTexture{nullptr};

        std::function<std::pair<int, int>()> sceneViewportSizeProvider{};

        std::function<void(const std::vector<std::filesystem::path>&)> openScriptFilesInEditor{};
        std::function<void(const std::filesystem::path&)> openAssetInEditor{};

        bool bEnableGui{true};
    };


    class DefaultEngineGuiContextFactory
    {
    public:
        explicit DefaultEngineGuiContextFactory(Core::SubsystemManager& subsystems);

        [[nodiscard]] DefaultEngineGuiContext CreateContext(const DefaultEngineGuiContextArgs& args) const;

    private:
        Core::SubsystemManager* subsystems_{nullptr};

        static std::function<std::pair<int, int>()> DefaultSizeProvider();
        static std::function<void(const std::vector<std::filesystem::path>&)> DefaultScriptOpener();
        static std::function<void(const std::filesystem::path&)> DefaultAssetOpener();
    };
}
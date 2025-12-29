#pragma once
#include <filesystem>
#include <functional>
#include <vector>


namespace BixEngine
{
    namespace Core
    {
        class Timer;
        class SubsystemManager;
    }

    namespace Game
    {
        class Actor;
        class SceneManager;
    }
}

struct SDL_Texture;

namespace BixEngine::Gui
{
    struct DefaultEngineGuiContext;

    
    
    
    struct DefaultEngineGuiContextArgs
    {
        
        Core::SubsystemManager& subsystems;

        
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
        Core::SubsystemManager& subsystems_;

        static std::function<std::pair<int, int>()> DefaultSizeProvider();
        static std::function<void(const std::vector<std::filesystem::path>&)> DefaultScriptOpener();
        static std::function<void(const std::filesystem::path&)> DefaultAssetOpener();
    };
}

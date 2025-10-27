#pragma once

#include <filesystem>
#include <functional>
#include <vector>

namespace BixEngine
{
    namespace Core { class Timer; class SubsystemManager; }
    namespace Game { class Actor; }
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
    };

    class DefaultEngineGuiContextFactory
    {
    public:
        explicit DefaultEngineGuiContextFactory(Core::SubsystemManager& subsystems);

        DefaultEngineGuiContext CreateContext(const DefaultEngineGuiContextArgs& args) const;

    private:
        Core::SubsystemManager& subsystems_;
    };
}


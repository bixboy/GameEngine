#pragma once

#include <memory>
#include <utility>

#include <SDL3/SDL_events.h>

namespace BixEngine
{
    namespace Core { class Window; class Timer; class SubsystemManager; }
    namespace Graphics { class Renderer; }
    namespace Gui { class GuiSystem; class GuiManager; class GuiPanel; }
    namespace Game { class Actor; }
}

struct SDL_Texture;

namespace BixEngine::Core
{
    class GuiModule
    {
    public:
        GuiModule() = default;
        ~GuiModule();

        GuiModule(const GuiModule&) = delete;
        GuiModule& operator=(const GuiModule&) = delete;
        GuiModule(GuiModule&&) noexcept = delete;
        GuiModule& operator=(GuiModule&&) noexcept = delete;

        bool Initialize(Window& window, Graphics::Renderer& renderer);
        void Shutdown() noexcept;
        bool IsInitialized() const noexcept;

        void ProcessEvent(const SDL_Event& event);
        void BeginFrame();
        void Render(SubsystemManager& subsystems);

        void SetupDefaultGuiPanels(SubsystemManager& subsystems, const float* lastDeltaTimePointer);

        bool EnsureSceneViewportTexture(Graphics::Renderer& renderer);
        void DestroySceneViewportTexture() noexcept;

        SDL_Texture* GetSceneViewportTexture() const noexcept { return sceneViewportTexture_; }
        std::pair<int, int> GetSceneViewportSize() const noexcept { return {sceneViewportWidth_, sceneViewportHeight_}; }
        Gui::GuiManager* GetGuiManager() noexcept { return guiManager_.get(); }

    private:
        std::unique_ptr<Gui::GuiSystem> guiSystem_{};
        std::unique_ptr<Gui::GuiManager> guiManager_{};

        Gui::GuiPanel* statsPanel_{nullptr};
        Gui::GuiPanel* outlinerPanel_{nullptr};
        Gui::GuiPanel* contentBrowserPanel_{nullptr};
        Gui::GuiPanel* inspectorPanel_{nullptr};
        Gui::GuiPanel* viewportPanel_{nullptr};

        Game::Actor* selectedActor_{nullptr};
        const float* lastDeltaTime_{nullptr};

        SDL_Texture* sceneViewportTexture_{nullptr};
        int sceneViewportWidth_{0};
        int sceneViewportHeight_{0};
        bool sceneViewportTextureErrorLogged_{false};
    };
}

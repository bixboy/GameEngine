#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <sstream>
#include <SDL3/SDL_events.h>

#include "Gui/Internal/NavBar/GuiAssetEditorManager.h"
#include "Gui/Internal/NavBar/GuiNavigationBar.h"


namespace BixEngine
{
    namespace Core
    {
        class Window;
        class Timer;
        class SubsystemManager;
    }

    namespace Graphics
    {
        class Renderer;
    }

    namespace Gui
    {
        class GuiSystem;
        class GuiManager;
        class GuiPanel;
        class ActorEditorController;
        class GuiLayoutManager;
        enum class EditorLayoutType;
    }

    namespace Game
    {
        class Actor;
    }
}

struct SDL_Texture;

namespace BixEngine::Core
{
     
    class GuiModule
    {
    public:
        enum class EngineState { Edit, Play, Pause, Step };

    public:
        GuiModule();
        ~GuiModule() noexcept;

         
        bool Initialize(Window& window, Graphics::Renderer& renderer);

         
        void Shutdown() noexcept;

         
        bool IsInitialized() const noexcept;

         
        bool ProcessEvent(const SDL_Event& event);

         
        void BeginFrame();

         
        void Render(SubsystemManager& subsystems);
        

         
        void SetupDefaultGuiPanels(SubsystemManager& subsystems, const float* lastDeltaTimePointer);

         
        bool IsMouseOverViewport() const noexcept;
        

         
        bool EnsureSceneViewportTexture(Graphics::Renderer& renderer);

         
        void DestroySceneViewportTexture() noexcept;

         
        SDL_Texture* GetSceneViewportTexture() const noexcept { return sceneViewportTexture_; }

         
        std::pair<int, int> GetSceneViewportSize() const noexcept
        {
            return {sceneViewportWidth_, sceneViewportHeight_};
        }

         
        GuiManager* GetGuiManager() noexcept { return guiManager_.get(); }

         
        GuiAssetEditorManager* GetAssetEditorManager() noexcept { return assetEditorManager_.get(); }
        const GuiAssetEditorManager* GetAssetEditorManager() const noexcept { return assetEditorManager_.get(); }
        
        
        void OnPlay();
        void OnStop();
        void OnPause();
        
        [[nodiscard]] EngineState GetEngineState() const noexcept { return m_EngineState; }


        
        
        
        

        std::unique_ptr<GuiSystem> guiSystem_;
        std::unique_ptr<GuiManager> guiManager_;
        std::unique_ptr<GuiLayoutManager> layoutManager_;
        std::unique_ptr<GuiNavigationBar> navigationBar_;

        SubsystemManager* subsystems_{nullptr};

        
        GuiPanel* statsPanel_{nullptr};
        GuiPanel* outlinerPanel_{nullptr};
        GuiPanel* contentBrowserPanel_{nullptr};
        GuiPanel* inspectorPanel_{nullptr};
        GuiPanel* viewportPanel_{nullptr};

        
        Game::Actor* selectedActor_{nullptr};
        const float* lastDeltaTime_{nullptr};

        
        SDL_Texture* sceneViewportTexture_{nullptr};
        int sceneViewportWidth_{0};
        int sceneViewportHeight_{0};
        bool sceneViewportTextureErrorLogged_{false};

        std::unique_ptr<GuiAssetEditorManager> assetEditorManager_{};
        std::vector<std::string> focusRequests_{};
        bool bInitialized_{false};

        void OpenAssetEditor(const std::filesystem::path& path);
        void CloseAssetEditor(const std::string& navigationId);

        void ProcessFocusRequests();
        void FocusSceneViewport();

    private:
        void DispatchPendingFileDrops();



        std::vector<std::filesystem::path> pendingDroppedFiles_{};

        EngineState m_EngineState{EngineState::Edit};
        std::stringstream m_SceneBackup;
    };
}

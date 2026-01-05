#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <sstream>
#include <SDL3/SDL_events.h>

#include "GuiSystem.h"
#include "Renderer.h"
#include "Gui/Core/GuiManager.h"
#include "NavBar/GuiAssetEditorManager.h"
#include "NavBar/GuiNavigationBar.h"
#include "Systems/Core/SubsystemManager.h"
#include "Systems/Core/Window.h"
#include "Gui/Internal/EditorSceneManager.h"
#include "Gui/Internal/MainMenuBar.h"


struct SDL_Texture;

namespace BixEngine::Gui
{
    class GuiModule
    {
    public:
        enum class EngineState { Edit, Play, Pause, Step };

    public:
        GuiModule();
        ~GuiModule() noexcept;

        // --- Lifecycle ---
        
        bool Initialize(Core::Window& window, Graphics::Renderer& renderer);
        void Shutdown() noexcept;
        bool IsInitialized() const noexcept;

        // --- Event & Update ---
        
        bool ProcessEvent(const SDL_Event& event);
        void BeginFrame();
        void Render(Core::SubsystemManager& subsystems);

        // --- Setup ---
        
        void SetupDefaultGuiPanels(Core::SubsystemManager& subsystems, const float* lastDeltaTimePointer);

        // --- Viewport & Texture Management ---
        
        bool IsMouseOverViewport() const noexcept;
        bool EnsureSceneViewportTexture(Graphics::Renderer& renderer);
        void DestroySceneViewportTexture() noexcept;

        SDL_Texture* GetSceneViewportTexture() const noexcept { return sceneViewportTexture_; }
        std::pair<int, int> GetSceneViewportSize() const noexcept { return {sceneViewportWidth_, sceneViewportHeight_}; }

        // --- Accessors ---
        
        GuiManager* GetGuiManager() noexcept { return guiManager_.get(); }
        GuiAssetEditorManager* GetAssetEditorManager() noexcept { return assetEditorManager_.get(); }

        // --- Engine State Control ---
        
        void OnPlay();
        void OnStop();
        void OnPause();
        [[nodiscard]] EngineState GetEngineState() const noexcept { return m_EngineState; }

        // --- Public Helpers ---
        
        void OpenAssetEditor(const std::filesystem::path& path);
        void CloseAssetEditor(const std::string& navigationId);

        void DispatchPendingFileDrops();
        void ProcessFocusRequests();
        void FocusSceneViewport();

    private:
        // Systems
        std::unique_ptr<GuiSystem> guiSystem_;
        std::unique_ptr<GuiManager> guiManager_;
        std::unique_ptr<GuiLayoutManager> layoutManager_;
        std::unique_ptr<GuiNavigationBar> navigationBar_;
        std::unique_ptr<GuiAssetEditorManager> assetEditorManager_;
        std::unique_ptr<EditorSceneManager> editorSceneManager_;
        std::unique_ptr<MainMenuBar> mainMenuBar_;

        Core::SubsystemManager* subsystems_{nullptr};

        // Pointers to Standard Panels
        GuiPanel* viewportPanel_{nullptr};

        // Context Data
        Game::Actor* selectedActor_{nullptr};
        const float* lastDeltaTime_{nullptr};

        // Rendering Data
        SDL_Texture* sceneViewportTexture_{nullptr};
        int sceneViewportWidth_{0};
        int sceneViewportHeight_{0};
        bool sceneViewportTextureErrorLogged_{false};

        // Internal State
        std::vector<std::string> focusRequests_{};
        std::vector<std::filesystem::path> pendingDroppedFiles_{};
        bool bInitialized_{false};

        EngineState m_EngineState{EngineState::Edit};
        std::stringstream m_SceneBackup;
    };
}
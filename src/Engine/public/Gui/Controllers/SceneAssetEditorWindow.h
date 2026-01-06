#pragma once

#include "Gui/Controllers/BaseAssetEditorWindow.h"
#include <memory> 
#include "Renderer.h"


namespace BixEngine::Game
{
    class Scene;
    class Actor;
}


namespace BixEngine::Gui
{
    class PrefabViewportPanel;

    class SceneAssetEditorWindow : public BaseAssetEditorWindow
    {
    public:
        struct SceneSharedState : SharedState
        {
            std::unique_ptr<Game::Scene> previewEnvironment; 
            std::shared_ptr<void> renderTarget;
             
            Game::Actor* selectedActor{nullptr};
            Game::Actor* cameraActor{nullptr};

            bool isControllingCamera = false;
            float cameraSpeed = 10.0f;
            float mouseSensitivity = 0.1f;
            SceneSharedState();
            ~SceneSharedState() override; 
        };

        SceneAssetEditorWindow(std::shared_ptr<SharedState> sharedState, PanelConfig config);
        ~SceneAssetEditorWindow() override;

    protected:
        void CreatePreviewEnvironment();
        
        // --- Core ---
        virtual void DrawViewport();
        virtual void DrawInspector() = 0;
        virtual void DrawOutline() {}     

        virtual void OnRenderScene(Graphics::Renderer& renderer) { (void)renderer; } 

        void UpdateCameraMovement(float deltaTime);
        void DrawEditorGrid(SDL_Renderer* renderer);
        void ResizeRenderTarget(SDL_Renderer* renderer, int width, int height);

        std::unique_ptr<PrefabViewportPanel> viewportPanel_;
    };
}
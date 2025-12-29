#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include "Containers/String.h"
#include "Gui/Controllers/BaseAssetEditorController.h"
#include "Framework/Scene.h"
#include "Framework/SceneManager.h"


namespace BixEngine::Gui
{
    class PrefabViewportPanel;
    class PrefabInspectorPanel;

    class ActorEditorController final : public BaseAssetEditorController
    {
    public:
        enum Section
        {
            Toolbar,
            Viewport,
            Outline,
            Inspector
        };
        
        struct ActorEditorSharedState : public SharedState
        {
             std::unique_ptr<Game::Scene> previewScene;
             std::unique_ptr<Game::SceneManager> sceneManager; // Wraps the preview scene
             Game::Actor* previewActor{nullptr};
             
             // Texture provider for the viewport (Scene renders to a framebuffer/texture)
             // We might need a FrameBuffer here or in the Scene?
             // For now, let's assume Scene has a way to get its render target or we create one.
             // For now, let's assume Scene has a way to get its render target or we create one.
             std::shared_ptr<void> renderTarget; // Placeholder
             Game::Actor* selectedActor{nullptr};
             Game::Actor* cameraActor{nullptr};
        };

        ActorEditorController(std::shared_ptr<SharedState> sharedState, Section section);
        ~ActorEditorController() override;
        static std::shared_ptr<SharedState> CreateSharedState(std::filesystem::path assetPath, String stableIdRoot, std::function<void()> onCloseRequest);

    protected:
        void DrawPanelContents(GuiPanel& panel) override;
        void OnPlayRequested() override;
        void OnSaveRequested() override;
        void OnCompileRequested() override;

    private:
        void DrawToolbar();
        void DrawViewport();
        void DrawOutline();
        void DrawInspector();

        void DrawViewportGrid_(const ImVec2& size);
        
        Section section_;
        
        // Sub-panels (Lazy initialized)
        std::unique_ptr<PrefabViewportPanel> viewportPanel_;
        std::unique_ptr<PrefabInspectorPanel> inspectorPanel_;
        std::unique_ptr<class PrefabOutlinerPanel> outlinerPanel_;
    };
}

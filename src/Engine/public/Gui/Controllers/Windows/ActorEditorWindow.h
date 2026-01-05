#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include "Containers/String.h"
#include "Gui/Controllers/SceneAssetEditorWindow.h"

namespace BixEngine::Game { class Scene; class Actor; }
namespace BixEngine::Graphics { class Renderer; }

namespace BixEngine::Gui
{
    class PrefabViewportPanel;
    class PrefabInspectorPanel;
    class PrefabOutlinerPanel;

    class ActorEditorWindow final : public SceneAssetEditorWindow
    {
    public:
        enum Section
        {
            Toolbar,
            Viewport,
            Outline,
            Inspector
        };
        
        struct ActorEditorSharedState : SceneSharedState
        {
            std::unique_ptr<Game::Actor> targetActor; 
            bool needsCameraFocus = true;
        };

        ActorEditorWindow(std::shared_ptr<SharedState> sharedState, Section section);
        ~ActorEditorWindow() override;

        ActorEditorWindow(const ActorEditorWindow&) = delete;
        ActorEditorWindow& operator=(const ActorEditorWindow&) = delete;
        
        ActorEditorWindow(ActorEditorWindow&&) = delete;
        ActorEditorWindow& operator=(ActorEditorWindow&&) = delete;
        // ------------------------------------------------
        
        static std::shared_ptr<SharedState> CreateSharedState(std::filesystem::path assetPath, String stableIdRoot, std::function<void()> onCloseRequest);

    protected:
        void DrawPanelContents(GuiPanel& panel) override;
        void OnSaveRequested() override;

        void OnPlayRequested() override {}
        void OnCompileRequested() override {}
        void DrawToolbarExtensions() override;
        
        void DrawInspector() override;
        void DrawOutline() override;
        
        void OnRenderScene(Graphics::Renderer& renderer) override;

    private:
        void DrawToolbar();
        void FocusCameraOnActor();

        Section section_;
        
        std::unique_ptr<PrefabInspectorPanel> inspectorPanel_;
        std::unique_ptr<PrefabOutlinerPanel> outlinerPanel_;
    };
}
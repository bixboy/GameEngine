#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include "Containers/String.h"
#include "Gui/Controllers/SceneAssetEditorWindow.h"


namespace BixEngine::Game { class Actor; class Component; }

namespace BixEngine::Gui
{
    class ComponentEditorWindow final : public SceneAssetEditorWindow
    {
    public:
        enum class Section { Toolbar, Inspector, Viewport };
        
        struct ComponentSharedState : SceneSharedState
        {
            std::unique_ptr<Game::Actor> previewHost;
            Game::Component* targetComponent = nullptr;
            bool needsCameraFocus = true;
        };
        
        ComponentEditorWindow(std::shared_ptr<SharedState> sharedState, Section section);
        ~ComponentEditorWindow() override;

        ComponentEditorWindow(const ComponentEditorWindow&) = delete;
        ComponentEditorWindow& operator=(const ComponentEditorWindow&) = delete;
        
        ComponentEditorWindow(ComponentEditorWindow&&) = delete;
        ComponentEditorWindow& operator=(ComponentEditorWindow&&) = delete;
        // ------------------------------------------------

        static std::shared_ptr<SharedState> CreateSharedState(std::filesystem::path assetPath, String stableIdRoot, std::function<void()> onCloseRequest);

    protected:
        void DrawPanelContents(GuiPanel& panel) override;
        void OnSaveRequested() override;
        
        void OnPlayRequested() override;
        void OnCompileRequested() override;
        
        void DrawInspector() override;
        void OnRenderScene(Graphics::Renderer& renderer) override;

    private:
        void DrawToolbar();
        void EnsureDummyVisuals();

        Section section_;
    };
}
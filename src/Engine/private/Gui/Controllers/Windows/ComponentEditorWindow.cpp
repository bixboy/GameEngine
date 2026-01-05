#include "Gui/Controllers/Windows/ComponentEditorWindow.h"
#include <utility>
#include "Debug/Logger.h"
#include "Gui/Utils/GuiHelpers.h"
#include "imgui.h"
#include "Framework/Actor.h"
#include "Components/Core/CameraComponent.h"
#include "Serializer/PrefabSerializer.h"


namespace BixEngine::Gui
{
    namespace
    {
        String BuildDisplayName(const std::filesystem::path& path)
        {
            return path.empty() ? "Component Prefab" : path.filename().string(); 
        }

        BaseAssetEditorWindow::PanelConfig MakePanelConfig(ComponentEditorWindow::Section section)
        {
            using Section = ComponentEditorWindow::Section;
            BaseAssetEditorWindow::PanelConfig config{};

            switch (section)
            {
            case Section::Toolbar:
                config.titlePrefix = "Toolbar";
                config.dockRegion = DockSpaceRegion::Top;
                config.dockCondition = ImGuiCond_Always;
                break;
                
            case Section::Viewport:
                config.titlePrefix = "Preview";
                config.dockRegion = DockSpaceRegion::Center;
                break;
                
            case Section::Inspector:
                config.titlePrefix = "Properties";
                config.dockRegion = DockSpaceRegion::Right;
                break;
            }
            
            config.stableIdSuffix = config.titlePrefix;
            return config;
        }
    }
    

    ComponentEditorWindow::ComponentEditorWindow(std::shared_ptr<SharedState> sharedState, Section section)
        : SceneAssetEditorWindow(std::move(sharedState), MakePanelConfig(section)), section_(section)
    {
    }

    ComponentEditorWindow::~ComponentEditorWindow() = default;

    std::shared_ptr<BaseAssetEditorWindow::SharedState> ComponentEditorWindow::CreateSharedState(std::filesystem::path assetPath, String stableIdRoot, 
        std::function<void()> onCloseRequest)
    {
        auto state = std::make_shared<ComponentSharedState>();
        state->assetPath = std::move(assetPath);
        state->stableIdRoot = std::move(stableIdRoot);
        state->onCloseRequest = std::move(onCloseRequest);
        state->assetDisplayName = BuildDisplayName(state->assetPath);
        state->assetTypeLabel = "Component";

        state->previewHost = std::make_unique<Game::Actor>("PreviewHost");

        auto loadedComp = Serialization::PrefabSerializer::LoadComponent(state->assetPath);
        if (loadedComp)
        {
             state->previewHost->AddComponent(std::move(loadedComp));
        }
        else
        {
             LOG_WARNING("Failed to load component or empty file: " + String(state->assetPath.string()));
        }

        if (!state->previewHost->GetComponents().empty())
        {
            state->targetComponent = state->previewHost->GetComponents().back().get();
        }

        return state;
    }


    void ComponentEditorWindow::DrawPanelContents(GuiPanel& panel)
    {
        auto state = std::static_pointer_cast<ComponentSharedState>(GetSharedState());
        if (!state)
            return;

        // 1. Initialisation Environment
        if (!state->previewEnvironment)
        {
            CreatePreviewEnvironment();
            
            state->selectedActor = state->previewHost.get();
            
            // Setup Caméra
            if (state->cameraActor)
            {
                state->cameraActor->SetPosition({0.0f, 2.0f, 5.0f}); 
                state->cameraActor->GetComponent<Game::CameraComponent>()->LookAt({0.0f, 0.0f, 0.0f});
            }
        }

        switch (section_)
        {
        case Section::Toolbar:
            DrawToolbar();
            break;
        case Section::Inspector:
            DrawInspector();
            break;
        case Section::Viewport:
            DrawViewport();
            break;
        }
    }

    void ComponentEditorWindow::DrawInspector()
    {
        auto state = std::static_pointer_cast<ComponentSharedState>(GetSharedState());
        if (!state || !state->targetComponent)
        {
            ImGui::TextDisabled("No Component Selected/Loaded");
            return;
        }

        ImGui::Text("Type: %s", state->targetComponent->GetTypeName().c_str());
        ImGui::Separator();
        state->targetComponent->DrawInspectorUI();
    }

    void ComponentEditorWindow::OnRenderScene(Graphics::Renderer& renderer)
    {
        SceneAssetEditorWindow::OnRenderScene(renderer);

        auto state = std::static_pointer_cast<ComponentSharedState>(GetSharedState());
        if (state && state->previewHost)
        {
            state->previewHost->ComputeWorldTransform(); 
            state->previewHost->Render(renderer);
        }
    }

    void ComponentEditorWindow::DrawToolbar() {DrawStandardToolbar();}
    void ComponentEditorWindow::OnPlayRequested() {}
    void ComponentEditorWindow::OnCompileRequested() {}
    
    void ComponentEditorWindow::OnSaveRequested()
    {
        if (auto state = std::static_pointer_cast<ComponentSharedState>(GetSharedState()))
        {
             if (state->targetComponent)
             {
                 if(Serialization::PrefabSerializer::SaveComponent(state->targetComponent, state->assetPath))
                 {
                     LOG_INFO("Saved Component: " + state->assetDisplayName);
                     state->isDirty = false;
                 }
                 else
                 {
                     LOG_ERROR("Failed to save Component: " + state->assetDisplayName);
                 }
             }
        }
    }
}
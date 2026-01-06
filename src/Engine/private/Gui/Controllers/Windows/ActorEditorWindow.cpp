#include "Gui/Controllers/Windows/ActorEditorWindow.h"
#include <utility>
#include "Debug/Logger.h"
#include "Gui/Utils/GuiHelpers.h"
#include "imgui.h"
#include "Gui/Core/GuiData.h"
#include "Gui/Panels/PrefabEditor/PrefabInspectorPanel.h"
#include "Gui/Panels/PrefabEditor/PrefabOutlinerPanel.h"
#include "Framework/Scene.h"
#include "Framework/Actor.h"
#include "Components/Core/CameraComponent.h"
#include "Serializer/PrefabSerializer.h"


namespace BixEngine::Gui
{
    namespace
    {
        String BuildDisplayName(const std::filesystem::path& path)
        {
            return path.empty() ? "Actor Prefab" : path.filename().string(); 
        }

        BaseAssetEditorWindow::PanelConfig MakePanelConfig(ActorEditorWindow::Section section)
        {
            using Section = ActorEditorWindow::Section;
            BaseAssetEditorWindow::PanelConfig config{};
            
            switch (section)
            {
            case Section::Toolbar:
                config.titlePrefix = "Toolbar";
                config.dockRegion = DockSpaceRegion::Top;
                break;
                
            case Section::Viewport:
                config.titlePrefix = "Viewport";
                config.dockRegion = DockSpaceRegion::Center;
                break;
                
            case Section::Outline:
                config.titlePrefix = "Hierarchy";
                config.dockRegion = DockSpaceRegion::Left;
                break;
                
            case Section::Inspector:
                config.titlePrefix = "Inspector";
                config.dockRegion = DockSpaceRegion::Right;
                break;
            }
            
            config.stableIdSuffix = config.titlePrefix;
            return config;
        }

        std::unique_ptr<Game::Actor> LoadActorBinary(const std::filesystem::path& path)
        {
            auto root = Serialization::PrefabSerializer::LoadPrefab(path);
            if (!root)
            {
                LOG_ERROR("LoadActorBinary: Failed to load prefab from " + String(path.string()));
                return std::make_unique<Game::Actor>("Empty Prefab");
            }
            
            return root;
        }
    }
    

    ActorEditorWindow::ActorEditorWindow(std::shared_ptr<SharedState> sharedState, Section section) : SceneAssetEditorWindow(std::move(sharedState),
        MakePanelConfig(section)), section_(section)
    {
    }

    ActorEditorWindow::~ActorEditorWindow() = default;

    std::shared_ptr<BaseAssetEditorWindow::SharedState> ActorEditorWindow::CreateSharedState(std::filesystem::path assetPath,
        String stableIdRoot, std::function<void()> onCloseRequest)
    {
        auto state = std::make_shared<ActorEditorSharedState>();
        state->assetPath = std::move(assetPath);
        state->stableIdRoot = std::move(stableIdRoot);
        state->onCloseRequest = std::move(onCloseRequest);
        state->assetDisplayName = BuildDisplayName(state->assetPath);
        state->assetTypeLabel = "Prefab";

        state->targetActor = LoadActorBinary(state->assetPath);
        state->selectedActor = state->targetActor.get();
        state->needsCameraFocus = true;
        
        return state;
    }

    void ActorEditorWindow::FocusCameraOnActor()
    {
        auto state = std::static_pointer_cast<ActorEditorSharedState>(GetSharedState());
        if (!state || !state->cameraActor || !state->targetActor)
            return;

        Math::Vector3 pos = state->targetActor->GetTransform().GetWorldPosition();
        
        float distance = 5.0f; 
        
        state->cameraActor->SetPosition({pos.x + distance, pos.y + distance * 0.5f, pos.z + distance}); 
        state->cameraActor->GetComponent<Game::CameraComponent>()->GetTransform().LookAt(pos);
    }

    void ActorEditorWindow::DrawPanelContents(GuiPanel& panel)
    {
        auto state = std::static_pointer_cast<ActorEditorSharedState>(GetSharedState());
        if (!state)
            return;

        if (!state->previewEnvironment)
        {
            CreatePreviewEnvironment();
        }

        if (state->needsCameraFocus && state->cameraActor)
        {
            FocusCameraOnActor();
            state->needsCameraFocus = false;
        }
        
        switch (section_)
        {
        case Toolbar:
            DrawToolbar();
            break;
            
        case Viewport:
            SceneAssetEditorWindow::DrawViewport();
            break;
            
        case Outline:
            DrawOutline();
            break;
            
        case Inspector:
            DrawInspector();
            break;
        }
    }

    void ActorEditorWindow::OnRenderScene(Graphics::Renderer& renderer)
    {
        SceneAssetEditorWindow::OnRenderScene(renderer);
        
        auto state = std::static_pointer_cast<ActorEditorSharedState>(GetSharedState());
        
        if (state && state->targetActor)
        {
            state->targetActor->ComputeWorldTransform(); 
            state->targetActor->Render(renderer);
        }
    }

    void ActorEditorWindow::DrawInspector()
    {
        auto state = std::static_pointer_cast<ActorEditorSharedState>(GetSharedState());
        
        if (!inspectorPanel_)
        {
            inspectorPanel_ = std::make_unique<PrefabInspectorPanel>(
                 [state]()
                 {
                     return state->previewEnvironment.get();
                 }, 
                 [state]()
                 {
                     return state->selectedActor;
                 },
                 [state](Game::Actor* a)
                 {
                     state->selectedActor = a;
                 }
            );
        }
        
        inspectorPanel_->Draw();
    }

    void ActorEditorWindow::DrawOutline()
    {
        auto state = std::static_pointer_cast<ActorEditorSharedState>(GetSharedState());
        
        if (!outlinerPanel_)
        {
            outlinerPanel_ = std::make_unique<PrefabOutlinerPanel>(
                [state]()
                {
                    return state->previewEnvironment.get();
                },
                [state]()
                {
                    return state->selectedActor;
                },
                [state](Game::Actor* a)
                {
                    state->selectedActor = a;
                },
                [state](const Game::Actor* a)
                {
                    return a != state->cameraActor;
                }
            );
            
            outlinerPanel_->SetRootActor(state->targetActor.get()); 
        }
        
        outlinerPanel_->Draw();
    }

    void ActorEditorWindow::DrawToolbar()
    {
        DrawStandardToolbar();
    }
    
    void ActorEditorWindow::DrawToolbarExtensions()
    {
        if (ImGui::Button("Focus (F)"))
        {
            FocusCameraOnActor();
        }
        
        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();

        if (ImGui::Button("Compile BP"))
        {
            OnCompileRequested();
        }
    }

    void ActorEditorWindow::OnSaveRequested()
    {
        if (const auto state = std::static_pointer_cast<ActorEditorSharedState>(GetSharedState()))
        {
            if (state->targetActor)
            {
                // Sauvegarde
                if (Serialization::PrefabSerializer::SavePrefab(state->targetActor.get(), state->assetPath))
                {
                    LOG_INFO("Saved Actor Prefab: " + state->assetDisplayName);
                    state->isDirty = false;
                }
                else
                {
                    LOG_ERROR("Failed to save Actor Prefab: " + state->assetDisplayName);
                }
            }
        }
    }
}
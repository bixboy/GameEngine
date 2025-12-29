#include "Gui/Controllers/ActorEditorController.h"
#include <fstream>
#include <utility>
#include "Debug/Logger.h"
#include "Gui/Utils/GuiHelpers.h"
#include "imgui.h"
#include "Gui/Core/GuiCommon.h"
#include "Gui/Core/EditorPreferences.h"
#include "Gui/Controllers/BaseAssetEditorController.h"
#include "Gui/Panels/PrefabEditor/PrefabViewportPanel.h"
#include "Gui/Panels/PrefabEditor/PrefabInspectorPanel.h"
#include "Gui/Panels/PrefabEditor/PrefabOutlinerPanel.h"
#include "Framework/Scene.h"
#include "Framework/Actor.h"
#include "Components/Core/CameraComponent.h"
#include "Framework/Actor.h"
#include "Framework/Actor.h"
#include "Serializer/SceneSerializer.h"
#include "Utils/FileIO/BinaryUtils.h"
#include "Utils/FileIO/PrefabUtils.h"
#include <SDL3/SDL.h>


namespace BixEngine::Gui
{
    namespace
    {
        String BuildDisplayName(const std::filesystem::path& path)
        {
            if (path.empty())
                return "Actor Prefab";
            return path.filename().generic_string();
        }

        BaseAssetEditorController::PanelConfig MakePanelConfig(ActorEditorController::Section section)
        {
            using Section = ActorEditorController::Section;
            BaseAssetEditorController::PanelConfig config{};

            switch (section)
            {
            case Section::Toolbar:
                config.titlePrefix = "Prefab Toolbar";
                config.dockRegion = DockSpaceRegion::Top;
                config.stableIdSuffix = "Toolbar";
                break;
            case Section::Viewport:
                config.titlePrefix = "Prefab Viewport";
                config.dockRegion = DockSpaceRegion::Center;
                config.stableIdSuffix = "Viewport";
                break;
            case Section::Outline: 
                config.titlePrefix = "Prefab Outline";
                config.dockRegion = DockSpaceRegion::Left;
                config.stableIdSuffix = "Outline";
                break;
            case Section::Inspector:
                config.titlePrefix = "Prefab Inspector";
                config.dockRegion = DockSpaceRegion::Right;
                config.stableIdSuffix = "Inspector";
                break;
            }
            return config;
        }

        
        std::unique_ptr<Game::Actor> LoadActorBinary(const std::filesystem::path& path)
        {
            auto root = BixEngine::PrefabUtils::PrefabSerializer::LoadPrefab(path);
            if (!root)
            {
                LOG_ERROR("LoadActorBinary: Failed to load prefab from " + path.string());
                return std::make_unique<Game::Actor>("Root(Fallback)");
            }
            return root;
        }
    }

    ActorEditorController::ActorEditorController(std::shared_ptr<SharedState> sharedState, Section section) 
        : BaseAssetEditorController(std::move(sharedState), MakePanelConfig(section)), section_(section)
    {
    }

    ActorEditorController::~ActorEditorController() = default;

    std::shared_ptr<ActorEditorController::SharedState> ActorEditorController::CreateSharedState(
        std::filesystem::path assetPath, String stableIdRoot, std::function<void()> onCloseRequest)
    {
        auto state = std::make_shared<ActorEditorSharedState>();
        state->assetPath = std::move(assetPath);
        state->stableIdRoot = std::move(stableIdRoot);
        state->onCloseRequest = std::move(onCloseRequest);
        state->assetDisplayName = BuildDisplayName(state->assetPath);
        state->assetTypeLabel = "Actor Prefab";

        
        state->previewScene = std::make_unique<Game::Scene>("PreviewScene");
        
        
        
        std::unique_ptr<Game::Actor> loadedActor = LoadActorBinary(state->assetPath);
        state->previewActor = loadedActor.get();
        state->selectedActor = state->previewActor;

        if (loadedActor)
        {
            
            
            
            
            struct HierarchyNode { Game::Actor* actor; Game::Actor* parent; };
            std::vector<HierarchyNode> hierarchy;
            std::vector<Game::Actor*> descendants;

            std::function<void(Game::Actor*)> collect = [&](Game::Actor* node)
            {
                for (auto* child : node->GetChildren())
                {
                    hierarchy.push_back({ child, node });
                    descendants.push_back(child);
                    collect(child);
                }
            };
            collect(state->previewActor);

            
            state->previewScene->AddActor(std::move(loadedActor));

            
            for (auto* child : descendants)
            {
                
                std::unique_ptr<Game::Actor> uChild(child);
                state->previewScene->AddActor(std::move(uChild));
            }

            
            for (const auto& node : hierarchy)
            {
                if (node.actor && node.parent)
                    node.actor->SetParent(node.parent);
            }
        }

        
        if (state->previewActor)
        {
            auto& camActor = state->previewScene->SpawnActor<Game::Actor>("EditorCamera");
            auto* camComp = camActor.AddComponent<Game::CameraComponent>();
            if (camComp)
            {
                camComp->SetAsMainCamera();
                camComp->Zoom = 1.0f; 
                
                Math::Vector3 pos = state->previewActor->GetTransform().GetPosition();
                
                
                
                
                
                
                
                
                
                
                camActor.SetPosition({pos.x, pos.y, 10.0f}); 
            }
            state->cameraActor = &camActor;
        }
        
        
        return state;
    }

    void ActorEditorController::DrawPanelContents(GuiPanel&  )
    {
        auto state = std::static_pointer_cast<ActorEditorSharedState>(GetSharedState());
        if (!state) return;

        switch (section_)
        {
        case Section::Toolbar:
            DrawToolbar();
            break;
            
        case Section::Viewport:
            DrawViewport();
            break;
            
        case Section::Outline:
            DrawOutline();
            break;
            
        case Section::Inspector:
            DrawInspector();
            break;
        }
    }

    void ActorEditorController::DrawViewport()
    {
        auto state = std::static_pointer_cast<ActorEditorSharedState>(GetSharedState());
        if (!state || !state->previewScene) return;

        if (!viewportPanel_)
        {
            viewportPanel_ = std::make_unique<PrefabViewportPanel>(
                
                [state]() -> SDL_Texture* {
                    return static_cast<SDL_Texture*>(state->renderTarget.get());
                },
                
                [state]() -> std::pair<int, int> {
                     if (auto* tex = static_cast<SDL_Texture*>(state->renderTarget.get()))
                     {
                         float w, h;
                         if (SDL_GetTextureSize(tex, &w, &h))
                             return { (int)w, (int)h };
                     }
                     return {0,0};
                },
                
                [state]() -> Game::Scene* {
                    return state->previewScene.get();
                },
                
                [state]() -> Game::Actor* { return state->selectedActor; },
                
                [state](Game::Actor* actor) { state->selectedActor = actor; }
            );
            
            
            
            
            
            
            
            
            
            
        }
        
        
        
        ImVec2 availContentSize = ImGui::GetContentRegionAvail();
        if (availContentSize.x < 1.0f) availContentSize.x = 1.0f;
        if (availContentSize.y < 1.0f) availContentSize.y = 1.0f;

        
        
        
        ImVec2 viewportSize = { 1920.0f, 1080.0f };

        SDL_Renderer* renderer = Graphics::Renderer::Get()->GetSDLRenderer();
        SDL_Texture* texture = static_cast<SDL_Texture*>(state->renderTarget.get());
        
        float texW = 0, texH = 0;
        if (texture) SDL_GetTextureSize(texture, &texW, &texH);
        
        
        if (!texture || (int)texW != (int)viewportSize.x || (int)texH != (int)viewportSize.y)
        {
             
             if (texture) state->renderTarget.reset(); 
             
             texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, (int)viewportSize.x, (int)viewportSize.y);
             if (texture)
             {
                 state->renderTarget = std::shared_ptr<void>(texture, [](void* ptr){
                      SDL_DestroyTexture(static_cast<SDL_Texture*>(ptr));
                 });
             }
        }
        
        if (state->renderTarget)
        {
             
             SDL_SetRenderTarget(renderer, static_cast<SDL_Texture*>(state->renderTarget.get()));
             SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); 
             SDL_RenderClear(renderer);
             
             
             
             
             
             
             state->previewScene->Render(*Graphics::Renderer::Get());
             
             SDL_SetRenderTarget(renderer, nullptr); 
        }

        
        
        
        
        viewportPanel_->Draw();
    }

    void ActorEditorController::DrawInspector()
    {
        auto state = std::static_pointer_cast<ActorEditorSharedState>(GetSharedState());
        if (!inspectorPanel_)
        {
            inspectorPanel_ = std::make_unique<PrefabInspectorPanel>(
                 
                 [state](){ return state->previewScene.get(); },
                 
                 [state](){ return state->selectedActor; },
                 [state](Game::Actor*  ){   }
            );
        }
        inspectorPanel_->Draw();

        
        if (state && state->previewActor)
        {
            ImGui::Separator();
            ImGui::Text("DEBUG INFO:");
            ImGui::Text("Actor Name: %s", state->previewActor->GetName().c_str());
            ImGui::Text("Component Count: %zu", state->previewActor->GetComponents().size());
            ImGui::Text("Type Name: %s", state->previewActor->GetTypeName().c_str());
            for(auto& c : state->previewActor->GetComponents())
            {
                ImGui::BulletText("%s", c->GetTypeName().c_str());
            }
        }
    }



    void ActorEditorController::DrawOutline()
    {
        auto state = std::static_pointer_cast<ActorEditorSharedState>(GetSharedState());
        if (!outlinerPanel_)
        {
            outlinerPanel_ = std::make_unique<PrefabOutlinerPanel>(
                
                [state](){ return state->previewScene.get(); },
                
                [state](){ return state->selectedActor; },
                
                [state](Game::Actor* a){ state->selectedActor = a; },
                
                [state](const Game::Actor* a){ return a != state->cameraActor; }
            );
        }
        outlinerPanel_->Draw();
    }

    void ActorEditorController::DrawToolbar()
    {
        DrawStandardToolbar();
        
    }

    void ActorEditorController::OnSaveRequested()
    {
        if (const auto state = std::static_pointer_cast<ActorEditorSharedState>(GetSharedState()))
        {
            if (state->previewActor)
            {
                if (BixEngine::PrefabUtils::PrefabSerializer::SavePrefab(state->previewActor, state->assetPath))
                {
                    LOG_INFO("Saved Actor Prefab: " + state->assetDisplayName);
                }
                else
                {
                    LOG_ERROR("Failed to save Actor Prefab: " + state->assetDisplayName);
                }
            }
        }
    }

    void ActorEditorController::OnPlayRequested() {}
    void ActorEditorController::OnCompileRequested() {}
    void ActorEditorController::DrawViewportGrid_(const ImVec2&  ) {} 
}

#include "Gui/Controllers/SceneAssetEditorWindow.h"
#include <SDL3/SDL.h>
#include "Gui/Panels/PrefabEditor/PrefabViewportPanel.h"
#include "Framework/Scene.h"
#include "Framework/Actor.h"
#include "Components/Core/CameraComponent.h"
#include <imgui.h>
#include <algorithm>


namespace BixEngine::Gui
{
    SceneAssetEditorWindow::SceneAssetEditorWindow(std::shared_ptr<SharedState> sharedState, PanelConfig config)
        : BaseAssetEditorWindow(std::move(sharedState), std::move(config))
    {
    }

    SceneAssetEditorWindow::SceneSharedState::SceneSharedState() = default;
    SceneAssetEditorWindow::SceneSharedState::~SceneSharedState() = default;

    SceneAssetEditorWindow::~SceneAssetEditorWindow() = default;

    void SceneAssetEditorWindow::CreatePreviewEnvironment()
    {
        auto state = std::static_pointer_cast<SceneSharedState>(GetSharedState());
        if (!state)
            return;

        // 1. Create environment
        if (!state->previewEnvironment)
            state->previewEnvironment = std::make_unique<Game::Scene>("EditorEnvironment");

        // 2. Setup Camera
        if (!state->cameraActor)
        {
            auto& camActor = state->previewEnvironment->SpawnActor<Game::Actor>("EditorCamera");
            auto* camComp = camActor.AddComponent<Game::CameraComponent>();
            
            if (camComp)
            {
                camComp->SetAsMainCamera();
                // camComp->SetNearClip(0.1f);
                // camComp->SetFarClip(1000.0f);
                camActor.SetPosition({0.0f, 5.0f, 10.0f}); 
                camComp->GetTransform().LookAt({0.0f, 0.0f, 0.0f});
            }
            state->cameraActor = &camActor;
        }
    }

    void SceneAssetEditorWindow::ResizeRenderTarget(SDL_Renderer* renderer, int width, int height)
    {
        auto state = std::static_pointer_cast<SceneSharedState>(GetSharedState());
        SDL_Texture* texture = static_cast<SDL_Texture*>(state->renderTarget.get());
        
        float texW = 0, texH = 0;
        if (texture)
            SDL_GetTextureSize(texture, &texW, &texH);

        if (!texture || static_cast<int>(texW) != width || static_cast<int>(texH) != height)
        {
            state->renderTarget.reset(); 
            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
            
            if (texture)
            {
                SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR); 
                state->renderTarget = std::shared_ptr<void>(texture,
                    [](void* ptr)
                    {
                        if(ptr)
                            SDL_DestroyTexture(static_cast<SDL_Texture*>(ptr));
                    });
            }
        }
    }

    void SceneAssetEditorWindow::UpdateCameraMovement(float deltaTime)
    {
        /* auto state = std::static_pointer_cast<SceneSharedState>(GetSharedState());
        if (!state || !state->cameraActor
            ) return;

        if (ImGui::IsMouseDown(ImGuiMouseButton_Right) && viewportPanel_ && viewportPanel_->IsHovered())
        {
            state->isControllingCamera = true;
            
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        {
            state->isControllingCamera = false;
        }

        if (state->isControllingCamera)
        {
            // --- Rotation ---
            
            float mouseX, mouseY;
            SDL_GetRelativeMouseState(&mouseX, &mouseY); 
            

            // --- Déplacement ---
            
            const bool* keys = SDL_GetKeyboardState(nullptr);
            
            // Vec3 forward = state->cameraActor->GetForward();
            // Vec3 right = state->cameraActor->GetRight();
            
            float speed = state->cameraSpeed * deltaTime;
            if (keys[SDL_SCANCODE_LSHIFT])
                speed *= 2.0f;

            if (keys[SDL_SCANCODE_W]) state->cameraActor->Move(forward * speed);
            if (keys[SDL_SCANCODE_S]) state->cameraActor->Move(forward * -speed);
            if (keys[SDL_SCANCODE_D]) state->cameraActor->Move(right * speed);
            if (keys[SDL_SCANCODE_A]) state->cameraActor->Move(right * -speed);
            if (keys[SDL_SCANCODE_Q]) state->cameraActor->Move({0, -speed, 0});
            if (keys[SDL_SCANCODE_E]) state->cameraActor->Move({0, speed, 0});
            
        }*/
    }

    void SceneAssetEditorWindow::DrawEditorGrid(SDL_Renderer* renderer)
    {
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
        
        int gridSize = 1000;
        int step = 100;
        
        // Ceci est une grille 2D overlay sur l'écran (HUD).
        // Pour une vraie grille 3D, tu devrais utiliser Graphics::Renderer::DrawLine3D si dispo.
        // Ici je dessine juste un repère visuel si on n'a pas de renderer debug 3D.
        
        // Appeler ton DebugRenderer 3D ici.
        // Graphics::Renderer::Get()->DrawGrid(100, 1.0f);
    }

    void SceneAssetEditorWindow::DrawViewport()
    {
        auto state = std::static_pointer_cast<SceneSharedState>(GetSharedState());
        if (!state || !state->previewEnvironment)
            return;
        
        UpdateCameraMovement(0.016f); 

        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        viewportSize.x = std::max(viewportSize.x, 1.0f);
        viewportSize.y = std::max(viewportSize.y, 1.0f);

        SDL_Renderer* renderer = Graphics::Renderer::Get()->GetSDLRenderer();
        
        ResizeRenderTarget(renderer, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));
        
        // --- RENDER PASS ---
        
        if (state->renderTarget)
        {
             SDL_Texture* target = static_cast<SDL_Texture*>(state->renderTarget.get());
             SDL_SetRenderTarget(renderer, target);
             
             // 1. Fond
             SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255); 
             SDL_RenderClear(renderer);
             
             auto* globalRenderer = Graphics::Renderer::Get();
             
             // 2. Rendu de la Scène
             if(globalRenderer)
             {
                 state->previewEnvironment->Render(*globalRenderer);
                 OnRenderScene(*globalRenderer);
             }
             
             // 3. Grid / Debug Overlay
             DrawEditorGrid(renderer);
             SDL_SetRenderTarget(renderer, nullptr);
        }

        // --- IMGUI VIEWPORT ---
        
        if (!viewportPanel_)
        {
            viewportPanel_ = std::make_unique<PrefabViewportPanel>(
                [state]() -> SDL_Texture*
                {
                    return static_cast<SDL_Texture*>(state->renderTarget.get());
                },
                [state]() -> std::pair<int, int>
                { 
                    float w,h; 
                    auto tex = static_cast<SDL_Texture*>(state->renderTarget.get());
                    
                    if(tex)
                    {
                        SDL_GetTextureSize(tex, &w, &h);   
                    }
                    else
                    {
                        w=0;
                        h=0;
                    }
                    
                    return {
                        static_cast<int>(w),
                        static_cast<int>(h)
                    }; 
                },
                [state]() -> Game::Scene*
                {
                    return state->previewEnvironment.get();
                },
                [state]() -> Game::Actor*
                {
                    return state->selectedActor;
                },
                [state](Game::Actor* actor)
                {
                    state->selectedActor = actor;
                }
            );
        }
        
        viewportPanel_->Draw();
    }
}
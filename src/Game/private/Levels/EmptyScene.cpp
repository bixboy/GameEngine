#include "Levels/EmptyScene.h"
#include "Framework/SceneRegistry.h"
#include "Entities/Player.h"
#include "Input.h"
#include "Debug/Logger.h"
#include "Framework/BGameplayStatics.h"


namespace BixEngine::Game
{
    REGISTER_SCENE(EmptyScene);

    namespace
    {
        constexpr Math::Vector3 kPlayerStart{320.0f, 240.0f, 0.0f};
    }

    EmptyScene::EmptyScene() : Scene("EmptyScene")
    {
    }

    void EmptyScene::HandleEvent(const SDL_Event& event)
    {
        Scene::HandleEvent(event);

        if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F1)
        {
            LOG_INFO("Reloading EmptyScene...");
            BGameplayStatics::ReloadScene();
        }
    }

    void EmptyScene::OnRuntimeUpdate(float deltaTime)
    {
        Scene::OnRuntimeUpdate(deltaTime);
        
    }


    void EmptyScene::OnEnter()
    {
        Scene::OnEnter();

        if (!HasRenderer() || !HasInputManager())
            LOG_WARNING("EmptyScene started without full context (renderer or input missing)");

        player_ = FindActorByType<Player>();

        if (!player_)
        {
             player_ = BGameplayStatics::SpawnActor<Player>(this);
             if (player_) 
                 player_->GetTransform().SetPosition(kPlayerStart);
        }

        if (player_ && HasInputManager())
        {
            player_->SetupInput(GetInputManager());
        }
            
        LOG_INFO("EmptyScene Entered.");
    }

    void EmptyScene::OnExit()
    {
        LOG_INFO("EmptyScene Exiting.");
        player_ = nullptr;
        Scene::OnExit();
    }
}
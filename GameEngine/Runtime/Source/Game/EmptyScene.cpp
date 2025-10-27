#include "Game/EmptyScene.h"
#include "Game/ActorSpawner.h"
#include "Game/Test/Player.h"
#include "Input/Input.h"
#include "Core/Math/Color.h"
#include "Core/Logger.h"


namespace BixEngine::Game
{
    namespace
    {
        constexpr Math::Vector3 kPlayerStart{320.0f, 240.0f, 0.0f};
        constexpr Math::Vector3 kPlayerSize{50.0f, 50.0f, 1.0f};
    }
    
    EmptyScene::EmptyScene(): Scene("EmptyScene")
    {
    }

    void EmptyScene::HandleEvent(const SDL_Event& event)
    {
        Scene::HandleEvent(event);
        
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F1)
            LOG_INFO("Reloading EmptyScene...");
    }

    void EmptyScene::Update(float deltaTime)
    {
        for (auto& actor : GetActors())
            actor->Update(deltaTime);
    }

    void EmptyScene::Render(Graphics::Renderer& renderer)
    {
        for (auto& actor : GetActors())
            actor->Render(renderer);
    }

    void EmptyScene::OnEnter()
    {
        Scene::OnEnter();
        
        LOG_INFO("Entering " + GetName());

        if (!HasRenderer() || !HasInputManager())
            LOG_WARNING("EmptyScene started without full context (renderer or input missing)");

        player_ = ActorSpawner::SpawnActor<Player>(
            Math::Vector3(kPlayerStart),
            Math::Vector3(kPlayerSize),
            Math::Color::Red().ToSDL()
        );

        if (player_ && HasInputManager())
            player_->SetupInput(GetInputManager());
    }

    void EmptyScene::OnExit()
    {
        LOG_INFO("Exiting " + GetName());
        player_ = nullptr;
        Scene::OnExit();
    }
}

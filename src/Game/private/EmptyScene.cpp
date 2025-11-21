#include "EmptyScene.h"
#include "SceneRegistry.h"
#include "Test/Player.h"
#include "Input.h"
#include "Math/Color.h"
#include "Logger.h"
#include "BGameplayStatics.h"


namespace BixEngine::Game
{
    REGISTER_SCENE(EmptyScene);

    namespace
    {
        constexpr Math::Vector3 kPlayerStart{320.0f, 240.0f, 0.0f};
        constexpr Math::Vector3 kPlayerSize{150.0f, 150.0f, 150.0f};
    }

    EmptyScene::EmptyScene() : Scene("EmptyScene")
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
        Scene::Update(deltaTime);

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

        if (!HasRenderer() || !HasInputManager())
            LOG_WARNING("EmptyScene started without full context (renderer or input missing)");

        player_ = FindActorByType<Player>();

        if (!player_)
        {
            player_ = BGameplayStatics::SpawnActor<Player>(this, kPlayerStart, kPlayerSize, Math::Color::Red().ToSDL());
        }

        if (player_ && HasInputManager())
            player_->SetupInput(GetInputManager());
    }

    void EmptyScene::OnExit()
    {
        player_ = nullptr;
        Scene::OnExit();
    }
}

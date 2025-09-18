#include "Game/EmptyScene.h"

#include <memory>

#include "Game/ActorSpawner.h"
#include "Game/Test/Player.h"
#include "Input/Input.h"
#include "Math/Color.h"

namespace Engine::Game
{
    EmptyScene::EmptyScene() : Scene("EmptyScene")
    {
        
    }

    void EmptyScene::HandleEvent(const SDL_Event&)
    {
        
    }

    void EmptyScene::Update(float deltaTime)
    {
        if (player_)
            player_->Update(deltaTime);
    }

    void EmptyScene::Render(Graphics::Renderer& renderer)
    {
        if (player_)
            player_->Render(renderer);
    }

    void EmptyScene::OnEnter()
    {
        Scene::OnEnter();

        player_ = ActorSpawner::SpawnActor<Player>(
            Math::Vector3(100, 100, 0),
            Math::Vector3(50, 50, 1),
            Math::Color::Red().ToSDL()
        );
        
        if (player_ && HasInputManager())
            player_->SetupInput(GetInputManager());
    }
}

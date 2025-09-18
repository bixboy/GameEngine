#include "Game/EmptyScene.h"
#include "Game/Test/Player.h"
#include "Input/Input.h"

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

        player_ = std::make_unique<Player>(
            Math::Vector3(300, 220, 0),
            Math::Vector3(50, 50, 1)
        );

        if (HasInputManager())
        {
            dynamic_cast<Player*>(player_.get())->SetupInput(GetInputManager());
        }
    }
}

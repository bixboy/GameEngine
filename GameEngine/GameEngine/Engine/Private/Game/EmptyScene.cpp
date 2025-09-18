#include "Game/EmptyScene.h"

#include <memory>

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
            Math::Vector3(300.0f, 220.0f, 0.0f),
            Math::Vector3(50.0f, 50.0f, 1.0f),
            SDL_Color{255, 255, 255, 255}
        );

        if (player_ && HasInputManager())
            player_->SetupInput(GetInputManager());
    }
}

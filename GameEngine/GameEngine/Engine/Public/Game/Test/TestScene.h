#pragma once
#include "Game/Scene.h"
#include "Game/Test/Player.h"

namespace Engine::Game
{
    class TestScene : public Scene
    {
    public:
        TestScene() = default;

        void OnEnter() override
        {
            player_ = std::make_unique<Player>(
                Math::Vector3(300.0f, 220.0f, 0.0f),
                Math::Vector3(50.0f, 50.0f, 1.0f)
            );

            player_->SetupInput(GetInputManager());
        }

        void Update(float dt) override
        {
            if (player_ && HasInputManager())
                player_->Update(dt);
        }

        void Render(Graphics::Renderer& renderer) override
        {
            if (player_)
                player_->Render(renderer);
        }
    
    private:
        std::unique_ptr<Player> player_;
    };
}
#include "Game/Test/TestScene.h"

#include <algorithm>
#include <memory>

#include "Game/Test/Player.h"
#include "Graphics/Renderer.h"
#include "Input/InputManager.h"
#include "Core/Math/Color.h"
#include "Core/Math/Math.h"

namespace BixEngine::Game
{
    namespace
    {
        constexpr Math::Vector3 kPlayerStart{320.0f, 240.0f, 0.0f};
        constexpr Math::Vector3 kPlayerSize{50.0f, 50.0f, 1.0f};
        constexpr SDL_Color kPlayerColor{255, 165, 0, 255};
    }

    TestScene::TestScene() : Scene("TestScene") {}

    TestScene::~TestScene() = default;

    void TestScene::OnEnter()
    {
        Scene::OnEnter();

        auto player = std::make_unique<Player>(kPlayerStart, kPlayerSize, kPlayerColor);
        player_ = player.get();
        AddActor(std::move(player));

        if (player_ && HasInputManager())
            player_->SetupInput(GetInputManager());
    }

    void TestScene::OnExit()
    {
        Scene::OnExit();

        if (player_)
        {
            auto& actors = GetActors();
            actors.erase(
                std::remove_if(
                    actors.begin(),
                    actors.end(),
                    [this](const std::unique_ptr<Actor>& actor)
                    {
                        return actor.get() == player_;
                    }),
                actors.end());

            player_ = nullptr;
        }
    }

    void TestScene::HandleEvent(const SDL_Event& event)
    {
        Scene::HandleEvent(event);
    }

    void TestScene::Update(float deltaTime)
    {
        if (player_)
            player_->Update(deltaTime);
    }

    void TestScene::Render(Graphics::Renderer& renderer)
    {
        if (player_)
            player_->Render(renderer);
    }
}


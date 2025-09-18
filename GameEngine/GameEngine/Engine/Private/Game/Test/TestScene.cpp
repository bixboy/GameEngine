#include "Game/Test/TestScene.h"

#include "Game/Test/Player.h"
#include "Graphics/Renderer.h"
#include "Input/InputManager.h"
#include "Math/Color.h"
#include "Math/Math.h"

namespace Engine::Game
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

        player_ = std::make_unique<Player>(kPlayerStart, kPlayerSize, kPlayerColor);

        if (player_ && HasInputManager())
            player_->SetupInput(GetInputManager());
    }

    void TestScene::OnExit()
    {
        Scene::OnExit();

        player_.reset();
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


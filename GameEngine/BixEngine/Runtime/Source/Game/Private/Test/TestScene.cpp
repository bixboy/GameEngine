#include "Bix/Game/Test/TestScene.h"

#include <algorithm>
#include <memory>

#include <array>

#include "Bix/Game/Test/Player.h"
#include "Bix/Graphics/Renderer.h"
#include "Bix/Input/InputManager.h"
#include "Bix/Math/Color.h"
#include "Bix/Math/Math.h"

namespace BixEngine::Game
{
    namespace
    {
        constexpr Math::Vector3 kPlayerStart{320.0f, 240.0f, 0.0f};
        constexpr Math::Vector3 kPlayerSize{50.0f, 50.0f, 1.0f};
        constexpr SDL_Color kPlayerColor{255, 165, 0, 255};
        constexpr const char* kTestSceneModule = "Test";
        constexpr ::BixEngine::Game::Scripting::ScriptMetadataEntry kTestSceneMetadata[] =
        {
            {"IncludePath", "Bix/Game/Test/TestScene.h"},
        };
    }

    BIX_DEFINE_SCRIPT_CLASS(TestScene, (::BixEngine::Game::Scripting::ScriptRegistrationDescriptor{
        .name = "TestScene",
        .moduleName = kTestSceneModule,
        .kind = ::BixEngine::Game::Scripting::ScriptKind::Scene,
        .category = "Examples",
        .tooltip = "Demonstration scene showcasing gameplay components.",
        .keywords = "Scene,Example,Test",
        .metadata = kTestSceneMetadata,
        .metadataCount = std::size(kTestSceneMetadata),
    }));

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


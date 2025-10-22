#include "Bix/Game/EmptyScene.h"
#include "Bix/Game/ActorSpawner.h"
#include "Bix/Game/Test/Player.h"
#include "Bix/Input/Input.h"
#include "Bix/Math/Color.h"

namespace BixEngine::Game
{
    namespace
    {
        constexpr const char* kEmptySceneModule = "Game";
    }

    BIX_DEFINE_SCRIPT_CLASS(EmptyScene, (::BixEngine::Game::Scripting::ScriptRegistrationDescriptor{
        .name = "EmptyScene",
        .moduleName = kEmptySceneModule,
        .kind = ::BixEngine::Game::Scripting::ScriptKind::Scene,
    }));

    BIX_IMPLEMENT_CLASS(EmptyScene);

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

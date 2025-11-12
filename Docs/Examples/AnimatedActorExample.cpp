// Example usage of the refactored sprite animation system.
#include "Game/Actor.h"
#include "Game/Components/SpriteComponent.h"
#include "Game/Components/SpriteAnimatorComponent.h"
#include "Engine/Ressources/ResourceManager.h"
#include "Engine/Ressources/SpriteAtlas.h"

using namespace BixEngine;

void CreateAnimatedActorExample(Game::Scene& scene)
{
    Game::Actor* actor = scene.SpawnActor<Game::Actor>();
    actor->AddComponent<Game::SpriteComponent>();
    auto* animatorComponent = actor->AddComponent<Game::SpriteAnimatorComponent>();

    const String atlasPath = "Assets/Characters/PinkMonster/PinkMonster.atlas";
    if (!animatorComponent->LoadSpriteAtlas(atlasPath, "Idle"))
        return;

    animatorComponent->Play("Idle");
}

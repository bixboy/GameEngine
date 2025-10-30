// Example usage of the enhanced sprite animation system.
#include "Game/Actor.h"
#include "Game/Components/SpriteComponent.h"
#include "Game/Components/SpriteAnimatorComponent.h"
#include "Engine/Render/SpriteAtlasUtils.h"
#include "Engine/Render/TextureManager.h"
#include "Graphics/Renderer.h"

using namespace BixEngine;

void CreateAnimatedActorExample(Game::Scene& scene, SDL_Renderer* renderer)
{
    Game::Actor* actor = scene.SpawnActor<Game::Actor>();
    auto* sprite = actor->AddComponent<Game::SpriteComponent>();
    auto* animatorComponent = actor->AddComponent<Game::SpriteAnimatorComponent>();

    auto texture = Render::TextureManager::Get().LoadTexture("Assets/Characters/Hero.png", renderer);
    if (!texture)
        return;

    const std::vector<Render::SpriteFrame> idleFrames = Render::SpriteAtlasUtils::LoadFramesFromAtlas(*texture, 4, 4);

    Render::SpriteAnimation idleAnimation;
    idleAnimation.Name = "Idle";
    idleAnimation.FrameRate = 8.0f;
    idleAnimation.Frames = idleFrames;

    animatorComponent->SetTargetSprite(sprite);
    animatorComponent->AddAnimation(idleAnimation);
    animatorComponent->Play("Idle");
}

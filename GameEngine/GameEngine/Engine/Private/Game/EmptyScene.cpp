#include "Game/EmptyScene.h"

#include <SDL3/SDL_keycode.h>

#include "Graphics/Renderer.h"

namespace Engine::Game {

EmptyScene::EmptyScene()
    : Scene("EmptyScene"),
      player_(300.f, 220.f, 50.f, 50.f, SDL_Color{255, 255, 255, 255}) {}

void EmptyScene::HandleEvent(const SDL_Event& /*event*/) {}

void EmptyScene::Update(float deltaTime) {
    Math::Vector3 position = player_.GetPosition();
    auto& input = GetInput();

    if (input.IsKeyDown(SDLK_z)) { position.y -= speed_ * deltaTime; }
    if (input.IsKeyDown(SDLK_s)) { position.y += speed_ * deltaTime; }
    if (input.IsKeyDown(SDLK_q)) { position.x -= speed_ * deltaTime; }
    if (input.IsKeyDown(SDLK_d)) { position.x += speed_ * deltaTime; }

    player_.SetPosition(position);
}

void EmptyScene::Render(Graphics::Renderer& renderer) {
    player_.Render(renderer);
}

}  // namespace Engine::Game

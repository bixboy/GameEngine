#pragma once

#include "Game/Entity.h"
#include "Game/Scene.h"

namespace Engine::Game {

class EmptyScene : public Scene {
public:
    EmptyScene();

    void HandleEvent(const SDL_Event& event) override;
    void Update(float deltaTime) override;
    void Render(Graphics::Renderer& renderer) override;

private:
    Entity player_{};
    float speed_ = 200.0f;
};

}  // namespace Engine::Game

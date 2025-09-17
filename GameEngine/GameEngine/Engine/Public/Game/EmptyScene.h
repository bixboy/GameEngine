#pragma once
#include "../Core/Scene.h"
#include "../Input/Input.h"
#include "Entity.h"

class EmptyScene : public Scene
{
public:
    EmptyScene();
    void HandleEvent(const SDL_Event& e) override;
    void Update(float dt) override;
    void Render(Renderer& renderer) override;

private:
    
    Input input;
    Entity player;
    float speed = 200.0f;
};

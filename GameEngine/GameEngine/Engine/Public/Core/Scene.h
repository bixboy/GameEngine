#pragma once
#include "string"
#include "SDL3/SDL.h"

class Renderer;

class Scene {
    
public:
    virtual ~Scene() = default;

    virtual void HandleEvent(const SDL_Event& e) = 0;
    virtual void Update(float dt) = 0;
    virtual void Render(Renderer& renderer) = 0;

    std::string getName() const { return sceneName; }

private:

    std::string sceneName = "None";
};

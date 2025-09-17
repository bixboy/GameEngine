#pragma once
#include "SDL3/SDL_stdinc.h"

class Timer
{
public:
    
    void Tick();
    float GetDeltaTime() const { return deltaTime; }

    float GetFPS() const { return (deltaTime > 0) ? 1.0f / deltaTime : 0.0f; }

private:
    Uint64 lastTicks = 0;
    float deltaTime = 0.0f;
};

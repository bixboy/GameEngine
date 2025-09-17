#include "../../Public/Core/Timer.h"
#include "SDL3/SDL_timer.h"

void Timer::Tick() {
    
    Uint64 current = SDL_GetTicks();
    
    if (lastTicks == 0)
        deltaTime = 0.016f;
    else
        deltaTime = (current - lastTicks) / 1000.0f;
    
    lastTicks = current;
}


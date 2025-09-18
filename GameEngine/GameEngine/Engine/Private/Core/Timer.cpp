#include "Core/Timer.h"

#include <SDL3/SDL_timer.h>

namespace Engine::Core {

void Timer::Tick() {
    const Uint64 current = SDL_GetPerformanceCounter();
    static const Uint64 frequency = SDL_GetPerformanceFrequency();

    if (lastCounter_ == 0) {
        deltaTime_ = 0.0f;
    } else {
        deltaTime_ = static_cast<float>(current - lastCounter_) / static_cast<float>(frequency);
    }

    lastCounter_ = current;
}

}  // namespace Engine::Core

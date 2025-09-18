#pragma once

#include <SDL3/SDL_stdinc.h>

namespace Engine::Core {

class Timer {
public:
    void Tick();

    [[nodiscard]] float GetDeltaTime() const noexcept { return deltaTime_; }
    [[nodiscard]] float GetFPS() const noexcept { return deltaTime_ > 0.0f ? 1.0f / deltaTime_ : 0.0f; }

private:
    Uint64 lastCounter_{0};
    float deltaTime_{0.0f};
};

}  // namespace Engine::Core

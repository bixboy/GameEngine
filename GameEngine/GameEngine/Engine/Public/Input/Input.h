#pragma once

#include <SDL3/SDL_events.h>

#include <unordered_set>

namespace Engine::Input {

class Input {
public:
    void ProcessEvent(const SDL_Event& event);

    [[nodiscard]] bool IsKeyDown(SDL_Keycode key) const noexcept;
    [[nodiscard]] bool IsQuitRequested() const noexcept { return quitRequested_; }

private:
    std::unordered_set<SDL_Keycode> pressedKeys_{};
    bool quitRequested_{false};
};

}  // namespace Engine::Input

#include "../../Public/Input/Input.h"
#include "SDL3/SDL_events.h"

void Input::ProcessEvent(const SDL_Event& e) {
    if (e.type == SDL_EVENT_QUIT) {
        quitRequested = true;
    }
    else if (e.type == SDL_EVENT_KEY_DOWN) {
        keys[e.key.key] = true;
    }
    else if (e.type == SDL_EVENT_KEY_UP) {
        keys[e.key.key] = false;
    }
}

bool Input::IsKeyDown(SDL_Keycode key) const {
    auto it = keys.find(key);
    return it != keys.end() && it->second;
}

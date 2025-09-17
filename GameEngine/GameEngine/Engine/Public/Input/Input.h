#pragma once
#include "unordered_map"
#include "SDL3/SDL_events.h"

class Input
{
    
public:
    void ProcessEvent(const SDL_Event& e);

    bool IsKeyDown(SDL_Keycode key) const;
    bool IsQuitRequested() const { return quitRequested; }

private:
    std::unordered_map<SDL_Keycode, bool> keys;
    bool quitRequested = false;
};

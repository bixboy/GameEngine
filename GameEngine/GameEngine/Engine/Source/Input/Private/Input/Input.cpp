#include "Input/Input.h"

namespace Engine::Input
{
    void Input::ProcessEvent(const SDL_Event& event)
    {
        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                quitRequested_ = true;
                break;
            
            case SDL_EVENT_KEY_DOWN:
                pressedKeys_.insert(event.key.key);
                break;
            
            case SDL_EVENT_KEY_UP:
                pressedKeys_.erase(event.key.key);
                break;
            
            default:
                break;
        }
    }

    bool Input::IsKeyDown(SDL_Keycode key) const noexcept
    {
        return pressedKeys_.find(key) != pressedKeys_.end();
    }
} 

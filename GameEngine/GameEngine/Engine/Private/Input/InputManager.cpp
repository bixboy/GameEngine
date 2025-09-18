#include "Input/InputManager.h"


void Engine::Input::InputManager::ProcessEvent(const SDL_Event& e)
{
    if (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP)
    {
        SDL_Keycode key = e.key.key;
        InputEvent eventType = (e.type == SDL_EVENT_KEY_DOWN) ? InputEvent::Pressed : InputEvent::Released;

        for (auto& binding : bindings_)
        {
            if (binding.key == key && binding.eventType == eventType)
                binding.callback();
        }
    }
}

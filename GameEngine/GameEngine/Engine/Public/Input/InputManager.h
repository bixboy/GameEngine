#pragma once
#include <unordered_map>
#include <vector>
#include <functional>
#include <string>
#include <SDL3/SDL.h>


namespace Engine::Input
{
    enum class InputEvent
    {
        Pressed,
        Released,
        Hold
    };

    class InputManager
    {
    public:
        void ProcessEvent(const SDL_Event& e);

        template <typename T>
        void BindAction(const std::string& actionName, SDL_Keycode key, InputEvent eventType, T* instance, void (T::*func)())
        {
            InputBinding binding;
            binding.actionName = actionName;
            binding.key = key;
            binding.eventType = eventType;

            binding.callback = [instance, func]() { (instance->*func)(); };

            bindings_.push_back(std::move(binding));
        }

    private:
        struct InputBinding
        {
            std::string actionName;
            SDL_Keycode key;
            
            InputEvent eventType;
            std::function<void()> callback;
        };

        std::vector<InputBinding> bindings_;
    };   
}
#include "Input/InputManager.h"

#include <unordered_map>

#include "Input/Input.h"

namespace Engine::Input
{
    namespace
    {
        bool IsKeyEvent(const SDL_Event& event) noexcept
        {
            return event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP;
        }
    }

    void InputManager::ProcessEvent(const SDL_Event& event)
    {
        if (!IsKeyEvent(event))
            return;

        const SDL_Keycode key = event.key.key;
        const bool isPressed = event.type == SDL_EVENT_KEY_DOWN;

        for (const auto& binding : actionBindings_)
        {
            if (binding.key != key)
                continue;

            switch (binding.eventType)
            {
                case InputEvent::Pressed:
                    if (isPressed && binding.callback)
                        binding.callback();
                    break;

                case InputEvent::Released:
                    if (!isPressed && binding.callback)
                        binding.callback();
                    break;

                case InputEvent::Hold:
                    break;
            }
        }
    }

    void InputManager::Update()
    {
        if (!input_)
            return;

        for (const auto& binding : actionBindings_)
        {
            if (binding.eventType != InputEvent::Hold)
                continue;

            if (binding.callback && input_->IsKeyDown(binding.key))
                binding.callback();
        }

        if (axisBindings_.empty())
            return;

        std::unordered_map<std::string, float> axisValues;
        axisValues.reserve(axisBindings_.size());

        for (const auto& binding : axisBindings_)
        {
            axisValues.try_emplace(binding.axisName, 0.0f);

            for (const auto& key : binding.keys)
            {
                if (!input_->IsKeyDown(key.key))
                    continue;

                axisValues[binding.axisName] += key.scale;
            }
        }

        for (const auto& binding : axisBindings_)
        {
            if (!binding.callback)
                continue;

            const float value = axisValues[binding.axisName];
            binding.callback(value);
        }
    }
}


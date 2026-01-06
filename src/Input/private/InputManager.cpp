#include "InputManager.h"
#include "Input.h"


namespace BixEngine::Input
{
    void InputManager::ProcessEvent(const SDL_Event& event)
    {
        InputKey currentInput;
        bool isPressed = false;
        bool isRelevant = false;

        if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
        {
            currentInput = InputKey::FromKey(event.key.key);
            isPressed = (event.type == SDL_EVENT_KEY_DOWN);
            isRelevant = true;
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            currentInput = InputKey::FromMouse(event.button.button);
            isPressed = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
            isRelevant = true;
        }

        if (!isRelevant)
            return;

        for (const auto& binding : actionBindings_)
        {
            if (binding.key == currentInput)
            {
                if (binding.key != currentInput) 
                    continue;

                const bool triggerPressed = (binding.eventType == InputEvent::Pressed && isPressed);
                const bool triggerReleased = (binding.eventType == InputEvent::Released && !isPressed);

                if (triggerPressed || triggerReleased)
                {
                    if (binding.callback)
                        binding.callback();
                }
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

            bool active = false;
            if (binding.key.IsKeyboard())
            {
                active = input_->IsKeyDown(binding.key.keyCode);
            }
            else if (binding.key.IsMouse())
            {
                active = input_->IsMouseButtonDown(binding.key.mouseButton);
            }

            if (active && binding.callback)
                binding.callback();
        }

        for (const auto& binding : axisBindings_)
        {
            float totalValue = 0.0f;

            for (const auto& keyEntry : binding.keys)
            {
                bool active = false;
                if (keyEntry.key.IsKeyboard())
                {
                    active = input_->IsKeyDown(keyEntry.key.keyCode);
                }
                else if (keyEntry.key.IsMouse())
                {
                    active = input_->IsMouseButtonDown(keyEntry.key.mouseButton);
                }

                if (active)
                    totalValue += keyEntry.scale;
            }

            if (binding.callback)
                binding.callback(totalValue);
        }
    }

    void InputManager::Reset() noexcept
    {
        ResetState();
        actionBindings_.clear();
        axisBindings_.clear();
    }

    void InputManager::ResetState() noexcept
    {
        if (input_)
            input_->ResetState();
    }

    void InputManager::UnbindAllForInstance(void* instance)
    {
        std::erase_if(actionBindings_, [instance](const ActionBinding& b)
            { 
                return b.instance == instance; 
            });
        
        std::erase_if(axisBindings_, [instance](const AxisBinding& b)
            { 
                return b.instance == instance; 
            });
    }
}
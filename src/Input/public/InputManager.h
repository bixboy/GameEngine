#pragma once
#include <functional>
#include <vector>
#include <algorithm>
#include <SDL3/SDL.h>
#include "Containers/String.h"

namespace BixEngine::Input
{
    class Input;

    enum class InputEvent
    {
        Pressed,
        Released,
        Hold
    };

    struct InputKey
    {
        SDL_Keycode keyCode = SDLK_UNKNOWN;
        int mouseButton = -1;

        bool IsKeyboard() const { return keyCode != SDLK_UNKNOWN; }
        bool IsMouse() const { return mouseButton != -1; }

        bool operator==(const InputKey& other) const {
            return keyCode == other.keyCode && mouseButton == other.mouseButton;
        }
        
        static InputKey FromKey(SDL_Keycode k) { return {k, -1}; }
        static InputKey FromMouse(int b) { return {SDLK_UNKNOWN, b}; }
    };

    class InputManager
    {
    public:
        InputManager() = default;
        void SetInputDevice(Input* input) noexcept { input_ = input; }

        void ProcessEvent(const SDL_Event& event);
        void Update();
        void Reset() noexcept;
        void ResetState() noexcept;

        // --- Templates ---

        template <typename T>
        void BindAction(String actionName, SDL_Keycode key, InputEvent eventType, T* instance, void (T::*func)())
        {
            ActionBinding binding;
            binding.actionName = std::move(actionName);
            binding.key = InputKey::FromKey(key);
            binding.eventType = eventType;
            binding.instance = instance;
            binding.callback = [instance, func]()
            {
                (instance->*func)();
            };

            actionBindings_.push_back(std::move(binding));
        }

        template <typename T>
        void BindAxis(String axisName, SDL_Keycode key, T* instance, void (T::*func)(float), float scale = 1.0f)
        {
            auto it = std::find_if(axisBindings_.begin(), axisBindings_.end(), 
                [&](const AxisBinding& b)
                { 
                    return b.axisName == axisName && b.instance == instance; 
                });

            if (it == axisBindings_.end())
            {
                AxisBinding binding;
                binding.axisName = std::move(axisName);
                binding.instance = instance;
                binding.callback = [instance, func](float val)
                {
                    (instance->*func)(val);
                };
                
                binding.keys.push_back({InputKey::FromKey(key), scale});
                axisBindings_.push_back(std::move(binding));
            }
            else
            {
                it->keys.push_back({InputKey::FromKey(key), scale});
            }
        }

        void UnbindAllForInstance(void* instance);

    private:
        struct ActionBinding
        {
            String actionName;
            InputKey key;
            InputEvent eventType;
            void* instance;
            std::function<void()> callback;
        };

        struct AxisKeyEntry
        {
            InputKey key;
            float scale;
        };

        struct AxisBinding
        {
            String axisName;
            void* instance;
            std::function<void(float)> callback;
            std::vector<AxisKeyEntry> keys;
        };

        Input* input_{nullptr};
        std::vector<ActionBinding> actionBindings_;
        std::vector<AxisBinding> axisBindings_;
    };
}
#pragma once
#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <typeindex>
#include <utility>
#include <vector>
#include <SDL3/SDL.h>

namespace Engine::Input
{
    class Input;

    enum class InputEvent
    {
        Pressed,
        Released,
        Hold
    };

    class InputManager
    {
        public:
            InputManager() = default;

            void SetInputDevice(Input* input) noexcept { input_ = input; }

            void ProcessEvent(const SDL_Event& event);
            void Update();

        
        // ======= Bind Action =======
            template <typename T>
            void BindAction(std::string actionName, SDL_Keycode key, InputEvent eventType, T* instance, void (T::*func)())
            {
                ActionBinding binding;
                binding.actionName = std::move(actionName);
                binding.key = key;
                binding.eventType = eventType;
                binding.callback = [instance, func]() { (instance->*func)(); };

                actionBindings_.push_back(std::move(binding));
            }

        
        // ======= Bind Axis =======
            template <typename T>
            void BindAxis(std::string axisName, SDL_Keycode key, T* instance, void (T::*func)(float), float scale = 1.0f)
            {
                const std::type_index typeIndex{typeid(T)};

                std::vector<std::byte> methodKey(sizeof(func));
                std::memcpy(methodKey.data(), &func, sizeof(func));

                auto matches = [&](const AxisBinding& binding)
                {
                    return binding.axisName == axisName &&
                           binding.instance == instance &&
                           binding.typeIndex == typeIndex &&
                           binding.methodKey == methodKey;
                };

                auto it = std::find_if(axisBindings_.begin(), axisBindings_.end(), matches);

                if (it == axisBindings_.end())
                {
                    AxisBinding binding;
                    binding.axisName = std::move(axisName);
                    binding.instance = instance;
                    binding.typeIndex = typeIndex;
                    binding.methodKey = std::move(methodKey);
                    binding.callback = [instance, func](float value) { (instance->*func)(value); };
                    binding.keys.push_back({key, scale});
                    axisBindings_.push_back(std::move(binding));
                }
                else
                {
                    it->keys.push_back({key, scale});
                }
            }

        
        // ======= Unbind Action =======
            void UnbindAction(const std::string& actionName, SDL_Keycode key, InputEvent eventType)
            {
                std::erase_if(actionBindings_, [&](const ActionBinding& binding)
                {
                    return binding.actionName == actionName
                    && binding.key == key
                    && binding.eventType == eventType;
                });
            }

        
        // ======= Unbind Axis =======
            void UnbindAxis(const std::string& axisName, SDL_Keycode key)
            {
                for (auto it = axisBindings_.begin(); it != axisBindings_.end();)
                {
                    auto& keys = it->keys;
                    std::erase_if(keys, [&](const AxisKey& axisKey)
                    {
                        return axisKey.key == key;
                    });

                    if (keys.empty())
                        it = axisBindings_.erase(it);
                    else
                        ++it;
                }
            }

        private:
            struct ActionBinding
            {
                std::string actionName;
                SDL_Keycode key{SDLK_UNKNOWN};
                InputEvent eventType{InputEvent::Pressed};
                std::function<void()> callback{};
            };

            struct AxisKey
            {
                SDL_Keycode key{SDLK_UNKNOWN};
                float scale{1.0f};
            };

            struct AxisBinding
            {
                std::string axisName;
                std::function<void(float)> callback{};
                void* instance{nullptr};
                std::type_index typeIndex{typeid(void)};
                std::vector<std::byte> methodKey{};
                std::vector<AxisKey> keys{};
            };

            Input* input_{nullptr};
            std::vector<ActionBinding> actionBindings_{};
            std::vector<AxisBinding> axisBindings_{};
    };
}


#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "Bix/Game/Actor.h"

namespace BixEngine::Game
{

    struct ComponentDescriptor
    {
        std::string name;
        std::function<void(Actor&)> createFunction;
    };

    class ComponentRegistry
    {
    public:
        using CreateFunction = std::function<void(Actor&)>;

        static ComponentRegistry& GetInstance();

        void RegisterComponent(std::string name, CreateFunction createFunction);

        template<typename TComponent, typename... TArgs>
        void RegisterComponent(std::string name, TArgs&&... args)
        {
            RegisterComponent(
                std::move(name),
                [arguments = std::tuple<std::decay_t<TArgs>...>(std::forward<TArgs>(args)...)](Actor& actor) mutable
                {
                    std::apply(
                        [&actor](auto&&... params)
                        {
                            actor.AddComponent<TComponent>(std::forward<decltype(params)>(params)...);
                        },
                        std::move(arguments));
                });
        }

        [[nodiscard]] std::vector<ComponentDescriptor> GetRegisteredComponents() const;

    private:
        ComponentRegistry() = default;

        mutable std::mutex mutex_;
        std::vector<ComponentDescriptor> descriptors_;
    };

    class ComponentRegistrar
    {
    public:
        ComponentRegistrar(std::string name, ComponentRegistry::CreateFunction createFunction);
    };
}

#define BIX_DETAIL_CONCAT_INNER(x, y) x##y
#define BIX_DETAIL_CONCAT(x, y) BIX_DETAIL_CONCAT_INNER(x, y)

// Registers a component using a custom factory lambda that receives the actor instance.
#define BIX_REGISTER_COMPONENT_WITH_FACTORY(ComponentType, DisplayName, FactoryLambda) \
    namespace \
    { \
        const ::BixEngine::Game::ComponentRegistrar \
            BIX_DETAIL_CONCAT(gBixComponentRegistrar_, __COUNTER__)(DisplayName, FactoryLambda); \
    }

// Convenience macro for registering components that can be constructed directly through Actor::AddComponent.
#define BIX_REGISTER_COMPONENT(ComponentType, DisplayName, ...) \
    BIX_REGISTER_COMPONENT_WITH_FACTORY( \
        ComponentType, \
        DisplayName, \
        [](::BixEngine::Game::Actor& actor) { actor.AddComponent<ComponentType>(__VA_ARGS__); })


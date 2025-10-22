#include "Bix/Game/Components/ComponentRegistry.h"

#include <algorithm>
#include <utility>

namespace BixEngine::Game
{
    ComponentRegistry& ComponentRegistry::GetInstance()
    {
        static ComponentRegistry instance;
        return instance;
    }

    void ComponentRegistry::RegisterComponent(std::string name, CreateFunction createFunction)
    {
        if (name.empty() || !createFunction)
        {
            return;
        }

        std::scoped_lock lock(mutex_);

        const auto it = std::find_if(
            descriptors_.begin(),
            descriptors_.end(),
            [&name](const ComponentDescriptor& descriptor)
            {
                return descriptor.name == name;
            });

        if (it != descriptors_.end())
        {
            it->createFunction = std::move(createFunction);
            return;
        }

        descriptors_.push_back(ComponentDescriptor{ std::move(name), std::move(createFunction) });
        std::sort(
            descriptors_.begin(),
            descriptors_.end(),
            [](const ComponentDescriptor& lhs, const ComponentDescriptor& rhs)
            {
                return lhs.name < rhs.name;
            });
    }

    std::vector<ComponentDescriptor> ComponentRegistry::GetRegisteredComponents() const
    {
        std::scoped_lock lock(mutex_);
        return descriptors_;
    }

    ComponentRegistrar::ComponentRegistrar(std::string name, ComponentRegistry::CreateFunction createFunction)
    {
        ComponentRegistry::GetInstance().RegisterComponent(std::move(name), std::move(createFunction));
    }
}

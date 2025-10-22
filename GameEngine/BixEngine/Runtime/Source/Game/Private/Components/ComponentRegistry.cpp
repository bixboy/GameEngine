#include "Bix/Game/Components/ComponentRegistry.h"

using namespace BixEngine::Game;

ComponentRegistry& ComponentRegistry::GetInstance()
{
    static ComponentRegistry instance;
    return instance;
}

void ComponentRegistry::RegisterComponent(std::string name, std::function<void(Actor&)> createFn)
{
    std::scoped_lock lock(mutex_);
    descriptors_.push_back({ std::move(name), std::move(createFn) });
}

std::vector<ComponentDescriptor> ComponentRegistry::GetRegisteredComponents() const
{
    std::scoped_lock lock(mutex_);
    return descriptors_;
}

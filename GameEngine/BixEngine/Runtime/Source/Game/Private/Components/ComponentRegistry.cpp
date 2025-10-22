#include "Bix/Game/Components/ComponentRegistry.h"

#include <unordered_set>

#include "Bix/Game/Actor.h"
#include "Bix/Game/Components/Component.h"
#include "Bix/Game/Scripting/ScriptReflection.h"

using namespace BixEngine::Game;

ComponentRegistry& ComponentRegistry::GetInstance()
{
    static ComponentRegistry instance;
    return instance;
}

void ComponentRegistry::RegisterComponent(std::string name, std::function<void(Actor&)> createFn)
{
    std::scoped_lock lock(mutex_);
    descriptors_.push_back({ std::move(name), std::move(createFn), nullptr });
}

std::vector<ComponentDescriptor> ComponentRegistry::GetRegisteredComponents() const
{
    std::vector<ComponentDescriptor> descriptors;
    {
        std::scoped_lock lock(mutex_);
        descriptors = descriptors_;
    }

    std::unordered_set<std::string> knownNames;
    knownNames.reserve(descriptors.size());
    for (const auto& descriptor : descriptors)
        knownNames.insert(descriptor.name);

    const auto scriptComponents = Scripting::ScriptRegistry::Get().GetClassesForEditor(Scripting::ScriptKind::Component);
    descriptors.reserve(descriptors.size() + scriptComponents.size());

    for (const auto* scriptClass : scriptComponents)
    {
        if (!scriptClass)
            continue;

        const std::string name = scriptClass->name.Std();
        if (!knownNames.insert(name).second)
            continue;

        ComponentDescriptor descriptor;
        descriptor.name = name;
        descriptor.scriptClass = scriptClass;
        descriptor.createFunction = [qualifiedName = descriptor.name](Actor& actor)
        {
            Scripting::ScriptInstantiationParams params;
            params.owner = &actor;

            auto component = Scripting::ScriptRegistry::Get().InstantiateAs<Component>(qualifiedName, params);
            if (component)
                actor.AddComponent(std::move(component));
        };

        descriptors.push_back(std::move(descriptor));
    }

    return descriptors;
}

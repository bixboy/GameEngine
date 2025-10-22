#pragma once
#include <vector>
#include <functional>
#include <string>
#include <mutex>

namespace BixEngine::Game
{
    namespace Scripting { struct ScriptClass; }

    class Actor;

    struct ComponentDescriptor
    {
        std::string name;
        std::function<void(Actor&)> createFunction;
        const Scripting::ScriptClass* scriptClass{nullptr};
    };

    class ComponentRegistry
    {
    public:
        static ComponentRegistry& GetInstance();

        void RegisterComponent(std::string name, std::function<void(Actor&)> createFn);
        std::vector<ComponentDescriptor> GetRegisteredComponents() const;

    private:
        mutable std::mutex mutex_;
        std::vector<ComponentDescriptor> descriptors_;
    };
}

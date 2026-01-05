#pragma once
#include <functional>
#include <memory>
#include <vector>
#include "Framework/Actor.h"


namespace BixEngine::Gui::ActorInspector
{
    class ActorInspectorSection
    {
    public:
        virtual ~ActorInspectorSection() = default;

        virtual void Draw(Game::Actor& actor) = 0;
    };

    using ActorInspectorSectionPtr = std::unique_ptr<ActorInspectorSection>;
    using ActorInspectorSectionList = std::vector<ActorInspectorSectionPtr>;
    using ActorInspectorSectionFactory = std::function<ActorInspectorSectionPtr()>;

    namespace SectionPriority
    {
        constexpr int Core = 0;
        constexpr int Transform = 100;
        constexpr int Scripts = 200;
        constexpr int Components = 300;
        constexpr int Debug = 1000;
    }

    ActorInspectorSectionList BuildActorInspectorSections();
    
    void RegisterActorInspectorSectionFactory(ActorInspectorSectionFactory factory, int priority = SectionPriority::Components);

    std::size_t GetRegisteredActorInspectorFactoryCount();
}
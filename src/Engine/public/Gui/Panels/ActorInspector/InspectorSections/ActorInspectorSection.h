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

    
    
    
    
    
    ActorInspectorSectionList BuildActorInspectorSections();

    void RegisterActorInspectorSectionFactory(ActorInspectorSectionFactory factory);

    std::size_t GetRegisteredActorInspectorFactoryCount();
}

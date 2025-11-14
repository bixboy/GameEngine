#pragma once

#include <functional>
#include <memory>
#include <vector>
#include "Actor.h"


namespace BixEngine::Gui::ActorInspector
{
    class ActorInspectorSection
    {
    public:
        virtual ~ActorInspectorSection() = default;

        virtual void Draw(Game::Actor& actor) = 0;
    };

    
    // -------------------------------------------------------------------------
    // Types utilitaires
    // -------------------------------------------------------------------------
    
    using ActorInspectorSectionPtr = std::unique_ptr<ActorInspectorSection>;

    using ActorInspectorSectionList = std::vector<ActorInspectorSectionPtr>;
    
    using ActorInspectorSectionFactory = std::function<ActorInspectorSectionPtr()>;

    
    // -------------------------------------------------------------------------
    // Fonctions globales du système d’inspection
    // -------------------------------------------------------------------------
    
    ActorInspectorSectionList BuildActorInspectorSections();

    void RegisterActorInspectorSectionFactory(ActorInspectorSectionFactory factory);

    std::size_t GetRegisteredActorInspectorFactoryCount();
}

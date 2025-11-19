#include "Gui/Panels/ActorInspector/InspectorSections/ActorInspectorSection.h"

#include "Gui/Panels/ActorInspector/InspectorSections/GeneralInspectorSection.h"
#include "Gui/Panels/ActorInspector/InspectorSections/TransformInspectorSection.h"
#include "Gui/Panels/ActorInspector/InspectorSections/ComponentInspectorSection.h"

#include <utility>


namespace BixEngine::Gui::ActorInspector
{
    namespace
    {
        std::vector<ActorInspectorSectionFactory>& GetRegisteredFactories()
        {
            static std::vector<ActorInspectorSectionFactory> factories;
            return factories;
        }
    }

    
    ActorInspectorSectionList BuildActorInspectorSections()
    {
        ActorInspectorSectionList sections;

        auto& factories = GetRegisteredFactories();
        sections.reserve(3 + factories.size());

        sections.emplace_back(std::make_unique<GeneralInspectorSection>());
        sections.emplace_back(std::make_unique<TransformInspectorSection>());
        sections.emplace_back(std::make_unique<ComponentInspectorSection>());

        for (auto& factory : factories)
        {
            if (!factory)
                continue;

            ActorInspectorSectionPtr section = factory();
            if (section)
                sections.emplace_back(std::move(section));
        }

        return sections;
    }

    void RegisterActorInspectorSectionFactory(ActorInspectorSectionFactory factory)
    {
        if (factory)
            GetRegisteredFactories().emplace_back(std::move(factory));
    }

    std::size_t GetRegisteredActorInspectorFactoryCount()
    {
        return GetRegisteredFactories().size();
    }
}

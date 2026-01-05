#include "Gui/Panels/ActorInspector/InspectorSections/ActorInspectorSection.h"
#include "Gui/Panels/ActorInspector/InspectorSections/GeneralInspectorSection.h"
#include "Gui/Panels/ActorInspector/InspectorSections/ComponentInspectorSection.h"
#include <utility>
#include <algorithm>


namespace BixEngine::Gui::ActorInspector
{
    namespace
    {
        struct RegisteredFactory
        {
            ActorInspectorSectionFactory factory;
            int priority;
        };

        std::vector<RegisteredFactory>& GetRegisteredFactories()
        {
            static std::vector<RegisteredFactory> factories;
            return factories;
        }
    }

    ActorInspectorSectionList BuildActorInspectorSections()
    {
        struct SectionEntry
        {
            ActorInspectorSectionPtr instance;
            int priority;
        };
        
        std::vector<SectionEntry> entries;
        entries.reserve(3 + GetRegisteredFactories().size());

        if (auto p = std::make_unique<GeneralInspectorSection>())
            entries.push_back({ std::move(p), SectionPriority::Core });

        if (auto p = std::make_unique<ComponentInspectorSection>())
            entries.push_back({ std::move(p), SectionPriority::Components });

        for (const auto& reg : GetRegisteredFactories())
        {
            if (!reg.factory)
                continue;

            if (auto section = reg.factory())
            {
                entries.push_back({ std::move(section), reg.priority });
            }
        }

        std::stable_sort(entries.begin(), entries.end(), 
            [](const SectionEntry& a, const SectionEntry& b)
            {
                return a.priority < b.priority;
            });

        ActorInspectorSectionList sections;
        sections.reserve(entries.size());
        
        for (auto& entry : entries)
        {
            sections.push_back(std::move(entry.instance));
        }

        return sections;
    }

    void RegisterActorInspectorSectionFactory(ActorInspectorSectionFactory factory, int priority)
    {
        if (factory)
        {
            GetRegisteredFactories().push_back({ std::move(factory), priority });
        }
    }

    std::size_t GetRegisteredActorInspectorFactoryCount()
    {
        return GetRegisteredFactories().size();
    }
}
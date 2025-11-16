#include "Gui/Panels/ActorInspector/InspectorSections/ActorInspectorSection.h"

#include "Gui/Panels/ActorInspector/InspectorSections/GeneralInspectorSection.h"
#include "Gui/Panels/ActorInspector/InspectorSections/TransformInspectorSection.h"
#include "Gui/Panels/ActorInspector/InspectorSections/ComponentInspectorSection.h"

#include <utility>


namespace BixEngine::Gui::ActorInspector
{
    // ==========================================================================
    // Stockage interne des factories (plugins)
    // ==========================================================================
    namespace
    {
        std::vector<ActorInspectorSectionFactory>& GetRegisteredFactories()
        {
            static std::vector<ActorInspectorSectionFactory> factories;
            return factories;
        }
    }

    
    // Construction des sections de base + plugins
    ActorInspectorSectionList BuildActorInspectorSections()
    {
        ActorInspectorSectionList sections;

        auto& factories = GetRegisteredFactories();
        sections.reserve(3 + factories.size());

        // --- Sections de base internes au moteur
        sections.emplace_back(std::make_unique<GeneralInspectorSection>());
        sections.emplace_back(std::make_unique<TransformInspectorSection>());
        sections.emplace_back(std::make_unique<ComponentInspectorSection>());

        // --- Sections ajoutées par plugins
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

    // Enregistrement d’une factory plugin
    void RegisterActorInspectorSectionFactory(ActorInspectorSectionFactory factory)
    {
        if (factory)
            GetRegisteredFactories().emplace_back(std::move(factory));
    }

    // Nombre de factories enregistrées
    std::size_t GetRegisteredActorInspectorFactoryCount()
    {
        return GetRegisteredFactories().size();
    }
}

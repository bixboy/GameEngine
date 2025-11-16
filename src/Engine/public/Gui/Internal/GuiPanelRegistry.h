#pragma once
#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include "Containers/String.h"
#include "Gui/Controllers/GuiPanelController.h"

namespace BixEngine::Gui
{
    class GuiPanel;

    /**
     * @brief Gestion interne des panneaux GUI et de leurs contrôleurs.
     */
    class GuiPanelRegistry
    {
    public:
        struct PanelEntry
        {
            std::unique_ptr<GuiPanel> panel;
            std::unique_ptr<GuiPanelController> controller;
            std::type_index panelType{typeid(void)};
        };

        using MapType = std::unordered_map<String, PanelEntry>;
        using Callback = std::function<void(GuiPanel&)>;

        GuiPanelRegistry() = default;
        ~GuiPanelRegistry() = default;
        

        /** Crée un nouveau panneau ou renvoie celui existant. */
        GuiPanel& AddPanel(String name, String title);

        template <typename PanelT, typename... Args>
        PanelT& AddPanelOfType(String name, String title, Args&&... args);

        /** Supprime le panneau et son contrôleur associé. */
        void RemovePanel(const String& name);

        /** Recherche un panneau par nom. */
        [[nodiscard]] GuiPanel* FindPanel(const String& name) noexcept;


        /** Retourne l'entrée complète d'un panneau via son nom. */
        [[nodiscard]] PanelEntry* FindPanelEntry(const String& name) noexcept;

        /** Retourne l'entrée complète d'un panneau via sa référence. */
        [[nodiscard]] PanelEntry* FindPanelEntry(GuiPanel& panel) noexcept;
        

        /** Récupère tous les panneaux. */
        [[nodiscard]] std::vector<GuiPanel*> GetAllPanels();
        

        /** Supprime tous les panneaux et vide les index. */
        void Clear();
        

        Callback OnPanelCreated = nullptr;
        Callback OnPanelRemoved = nullptr;

    private:
        MapType panels_; ///< Conteneur principal (nom → entrée)
        std::unordered_map<GuiPanel*, String> panelToName_;

        /** Ajoute un panneau à l’index inverse. */
        void RegisterPanelIndex_(GuiPanel& panel, const String& name);

        /** Retire un panneau de l’index inverse. */
        void UnregisterPanelIndex_(GuiPanel& panel);
    };
}

namespace BixEngine::Gui
{
    template <typename PanelT, typename... Args>
    PanelT& GuiPanelRegistry::AddPanelOfType(String name, String title, Args&&... args)
    {
        static_assert(std::is_base_of_v<GuiPanel, PanelT>, "PanelT must derive from GuiPanel");

        if (auto it = panels_.find(name); it != panels_.end())
        {
            auto& entry = it->second;
            if (!entry.panel)
            {
                entry.panel = std::make_unique<PanelT>(String{name}, String{title}, std::forward<Args>(args)...);
                entry.panelType = std::type_index(typeid(PanelT));
                
                PanelT& panelRef = static_cast<PanelT&>(*entry.panel);
                RegisterPanelIndex_(panelRef, it->first);
                
                if (OnPanelCreated)
                    OnPanelCreated(panelRef);
                
                return panelRef;
            }

            if (entry.panelType != std::type_index(typeid(PanelT)))
                throw std::runtime_error("GuiPanelRegistry::AddPanelOfType — panel already exists with a different type.");

            auto* existing = static_cast<PanelT*>(entry.panel.get());
            existing->SetTitle(std::move(title));
            
            return *existing;
        }

        PanelEntry entry{};
        entry.panel = std::make_unique<PanelT>(String{name}, String{title}, std::forward<Args>(args)...);
        entry.panelType = std::type_index(typeid(PanelT));
        PanelT& panelRef = static_cast<PanelT&>(*entry.panel);
        RegisterPanelIndex_(panelRef, name);

        const String key = name;
        panels_.emplace(key, std::move(entry));

        if (OnPanelCreated)
            OnPanelCreated(panelRef);

        return panelRef;
    }
}

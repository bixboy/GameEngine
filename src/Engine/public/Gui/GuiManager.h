#pragma once
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include "Containers/String.h"
#include "Gui/GuiDocking.h"
#include "Gui/Internal/GuiPanelRegistry.h"
#include "imgui.h"


namespace BixEngine::Gui
{
    class GuiSystem;
    class GuiPanel;
    class GuiPanelController;
    class GuiPanelBase;

    template <typename ControllerT>
    struct PanelRegistration
    {
        GuiPanel& panel;
        ControllerT& controller;
    };

    /**
     * @brief Central entry point that orchestrates panel creation, registration and
     * controller binding for the editor GUI.
     */
    class GuiManager
    {
    public:
        explicit GuiManager(GuiSystem& guiSystem);
        ~GuiManager();

        GuiManager(const GuiManager&) = delete;
        GuiManager& operator=(const GuiManager&) = delete;
        GuiManager(GuiManager&&) noexcept = delete;
        GuiManager& operator=(GuiManager&&) noexcept = delete;

        GuiPanel& CreatePanel(String name, String title);

        template <typename PanelT, typename... Args>
        PanelT& CreatePanelOfType(String name, String title, Args&&... args)
        {
            return registry_.AddPanelOfType<PanelT>(std::move(name), std::move(title), std::forward<Args>(args)...);
        }

        void RemovePanel(const String& name);
        void RemovePanels(std::span<GuiPanel*> panels);

        [[nodiscard]] GuiPanel* FindPanel(const String& name) noexcept;
        [[nodiscard]] const GuiPanel* FindPanel(const String& name) const noexcept;

        void SetPanelDockingArea(const String& name, DockSpaceRegion area, ImGuiCond condition = ImGuiCond_FirstUseEver);
        void SetPanelDockingArea(GuiPanel& panel, DockSpaceRegion area, ImGuiCond condition = ImGuiCond_FirstUseEver);

        void DrawAll();
        [[nodiscard]] std::vector<GuiPanel*> GetPanels();
        [[nodiscard]] std::vector<const GuiPanel*> GetPanels() const;

        GuiPanelController& AttachController(const String& name, std::unique_ptr<GuiPanelController> controller);
        GuiPanelController& AttachController(GuiPanel& panel, std::unique_ptr<GuiPanelController> controller);
        void DetachController(const String& name);
        void DetachController(GuiPanel& panel);

        [[nodiscard]] GuiPanelController* GetController(const String& name) noexcept;
        [[nodiscard]] const GuiPanelController* GetController(const String& name) const noexcept;

        template <typename T>
        T* GetControllerAs(const String& name) noexcept
        {
            if (auto* base = GetController(name))
                return dynamic_cast<T*>(base);
            return nullptr;
        }

        template <typename ControllerT, typename... Args>
        PanelRegistration<ControllerT> RegisterUtilityPanel(String name, String title, Args&&... args)
        {
            GuiPanel& panel = CreatePanel(std::move(name), std::move(title));
            auto controller = std::make_unique<ControllerT>(std::forward<Args>(args)...);
            ControllerT& controllerRef = static_cast<ControllerT&>(AttachController(panel, std::move(controller)));
            return {panel, controllerRef};
        }

        /** Registers a panel type inside the static factory. */
        template <typename PanelT, typename... Args>
        static void RegisterPanel(const String& displayName, Args&&... args)
        {
            auto& registry = StaticPanelRegistry_();
            RegisteredPanel entry{};
            entry.displayName = displayName;
            entry.identifier = SanitizeIdentifier_(displayName);

            using TupleType = std::tuple<std::decay_t<Args>...>;
            TupleType argsTuple(std::forward<Args>(args)...);
            entry.factory = [argsTuple]() mutable -> std::unique_ptr<GuiPanelBase>
            {
                return std::apply([](auto&... captured)
                {
                    return std::make_unique<PanelT>(captured...);
                }, argsTuple);
            };

            registry[displayName.Std()] = std::move(entry);
        }

        /** Removes a panel factory entry from the static registry. */
        static void UnregisterPanel(const String& displayName);

        /** Creates a panel instance using the static registry. */
        GuiPanelBase* CreatePanelByName(const String& displayName);

        std::function<void(GuiPanel&)> OnPanelCreated;
        std::function<void(GuiPanel&)> OnPanelRemoved;

    private:
        struct RegisteredPanel
        {
            String displayName;
            String identifier;
            std::function<std::unique_ptr<GuiPanelBase>()> factory;
        };

        static std::unordered_map<std::string, RegisteredPanel>& StaticPanelRegistry_();
        static String SanitizeIdentifier_(const String& name);

        GuiSystem* guiSystem_{nullptr};
        GuiPanelRegistry registry_;

        void AttachDrawFunction_(GuiPanelRegistry::PanelEntry& entry);
    };
}


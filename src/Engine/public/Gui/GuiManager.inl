#pragma once
#include "Gui/GuiManager.h"


namespace BixEngine::Gui
{
    template <typename PanelT, typename ... Args>
    PanelT& GuiManager::CreatePanelOfType(String name, String title, Args&&... args)
    {
        return registry_.AddPanelOfType<PanelT>(std::move(name), std::move(title), std::forward<Args>(args)...);
    }
    
    template <typename ControllerT, typename ... Args>
    ControllerT& GuiManager::OpenPanel(String name, String title, Args&&... args)
    {
        GuiPanel& panel = CreatePanel(std::move(name), std::move(title));
        auto controller = std::make_unique<ControllerT>(std::forward<Args>(args)...);
        ControllerT& ref = static_cast<ControllerT&>(AttachController(panel, std::move(controller)));
        
        return ref;
    }

    template <typename T>
    T* GuiManager::GetControllerAs(const String& name) noexcept
    {
        if (auto* base = GetController(name))
            return dynamic_cast<T*>(base);
            
        return nullptr;
    }

    template <typename ControllerT, typename ... Args>
    PanelRegistration<ControllerT> GuiManager::RegisterUtilityPanel(String name, String title, Args&&... args)
    {
        GuiPanel& panel = CreatePanel(std::move(name), std::move(title));
        auto controller = std::make_unique<ControllerT>(std::forward<Args>(args)...);
        ControllerT& controllerRef = static_cast<ControllerT&>(AttachController(panel, std::move(controller)));
        
        return {panel, controllerRef};
    }

    template <typename PanelT, typename ... Args>
   void GuiManager::RegisterPanel(const String& displayName, Args&&... args)
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
            { return std::make_unique<PanelT>(captured...); }, argsTuple);
        };

        registry[displayName.Std()] = std::move(entry);
    }

template <typename ControllerT, typename... Args>
    ControllerT& GuiManager::OpenAssetEditor(const std::filesystem::path& assetPath,
                                             BaseAssetEditorController::PanelConfig config,
                                             Args&&... args)
    {
        auto& registry = assetEditors_;
        const auto normalized = AssetEditorRegistry::NormalizePath(assetPath);

        // ─────────────────────────────
        // 1. Si déjà ouvert → focus + retour
        // ─────────────────────────────
        if (const auto* existing = registry.FindEntry(normalized))
        {
            if (existing->panel)
            {
                existing->panel->SetVisible(true);
                existing->panel->RequestFocus();
            }

            if (auto* controller = dynamic_cast<ControllerT*>(existing->controller))
                return *controller;

            // type différent → on ferme la vieille entrée
            registry.CloseEditor(normalized);
        }

        // ─────────────────────────────
        // 2. Préparation du SharedState
        // ─────────────────────────────
        auto sharedState = std::make_shared<BaseAssetEditorController::SharedState>();
        sharedState->assetPath        = normalized;
        sharedState->assetDisplayName = AssetEditorRegistry::ExtractDisplayName(normalized);
        sharedState->stableIdRoot     = AssetEditorRegistry::MakePanelName(normalized);
        sharedState->assetTypeLabel   =
            config.titlePrefix.IsEmpty() ? String{"Asset"} : config.titlePrefix;

        String panelName = sharedState->stableIdRoot;
        String title = sharedState->assetDisplayName.IsEmpty()
                           ? String{"Asset Editor"}
                           : sharedState->assetDisplayName;

        sharedState->onCloseRequest = [this, normalized]()
        {
            assetEditors_.CloseEditor(normalized);
        };

        // ─────────────────────────────
        // 3. Création réelle du panel + controller
        // ─────────────────────────────
        ControllerT& controller =
            OpenPanel<ControllerT>(panelName,
                                   title,
                                   sharedState,
                                   config,
                                   std::forward<Args>(args)...);

        registry.Register(normalized, controller.GetPanel(), controller, sharedState);
        return controller;
    }   
}
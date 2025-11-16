#pragma once
#include "Gui/Internal/GuiPanel.h"
#include "Utils/FilesUtils.h"
#include "Utils/StringUtils.h"

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

    template <typename ControllerT, typename... Args>
    PanelRegistration<ControllerT> GuiManager::RegisterPanel(PanelDescriptor descriptor, Args&&... args)
    {
        if (descriptor.identifier.IsEmpty())
            throw std::invalid_argument("GuiManager::RegisterPanel — identifier cannot be empty");

        if (descriptor.title.IsEmpty())
            descriptor.title = descriptor.identifier;

        GuiPanel& panel = CreatePanel(std::move(descriptor.identifier), std::move(descriptor.title));
        ApplyPanelDescriptor_(panel, descriptor);

        auto controller = std::make_unique<ControllerT>(std::forward<Args>(args)...);
        ControllerT& controllerRef = static_cast<ControllerT&>(AttachController(panel, std::move(controller)));

        if (descriptor.requestFocus)
            panel.RequestFocus();

        return {panel, controllerRef};
    }

template <typename ControllerT, typename... Args>
    ControllerT& GuiManager::OpenAssetEditor(const std::filesystem::path& assetPath, BaseAssetEditorController::PanelConfig config, Args&&... args)
    {
        auto& registry = assetEditors_;
        const auto normalized = FileUtils::NormalizePath(assetPath);
        
        if (const auto* existing = registry.FindEntry(normalized))
        {
            if (existing->panel)
            {
                existing->panel->SetVisible(true);
                existing->panel->RequestFocus();
            }

            if (auto* controller = dynamic_cast<ControllerT*>(existing->controller))
                return *controller;

            registry.CloseEditor(normalized);
        }
        
        auto sharedState = std::make_shared<BaseAssetEditorController::SharedState>();
        sharedState->assetPath = normalized;
        sharedState->assetDisplayName = FileUtils::ExtractDisplayName(normalized);
        sharedState->stableIdRoot = StringUtils::MakeSafeIdentifier(normalized.generic_string());
        sharedState->assetTypeLabel  = config.titlePrefix.IsEmpty() ? String{"Asset"} : config.titlePrefix;

        String panelName = sharedState->stableIdRoot;
        String title = sharedState->assetDisplayName.IsEmpty() ? String{"Asset Editor"} : sharedState->assetDisplayName;

        sharedState->onCloseRequest = [this, normalized]()
        {
            assetEditors_.CloseEditor(normalized);
        };
        
        ControllerT& controller = OpenPanel<ControllerT>(panelName, title, sharedState, config, std::forward<Args>(args)...);

        registry.Register(normalized, controller.GetPanel(), controller, sharedState);
        return controller;
    }   
}

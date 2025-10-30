#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>

#include "Core/Containers/String.h"
#include "Engine/Gui/Controllers/GuiPanelController.h"
#include "Engine/Gui/Core/GuiLayoutManager.h"

#include "imgui.h"
#include "Engine/Gui/Core/GuiDocking.h"

namespace BixEngine::Gui
{
    class BaseAssetEditorController : public GuiPanelController
    {
    public:
        struct SharedState
        {
            std::filesystem::path assetPath{};
            String assetDisplayName{};
            String stableIdRoot{};
            String assetTypeLabel{};
            std::function<void()> onCloseRequest{};
            std::string primaryClassName{};
            std::string includePath{};
        };

        struct PanelConfig
        {
            String titlePrefix{};
            DockSpaceRegion dockRegion{DockSpaceRegion::Center};
            ImGuiCond dockCondition{ImGuiCond_FirstUseEver};
            String stableIdSuffix{};
        };

        BaseAssetEditorController(std::shared_ptr<SharedState> sharedState, PanelConfig config);
        ~BaseAssetEditorController() override = default;

        [[nodiscard]] std::shared_ptr<SharedState> GetSharedState() const noexcept { return state_; }

    protected:
        void DrawStandardToolbar();

        virtual void DrawPanelContents(GuiPanel& panel) = 0;
        virtual void OnPlayRequested();
        virtual void OnSaveRequested();
        virtual void OnCompileRequested();

        [[nodiscard]] const PanelConfig& GetPanelConfig() const noexcept { return config_; }

    private:
        void OnAttach(GuiPanel& panel) override;
        void OnDetach(GuiPanel& panel) override;
        void OnDraw(GuiPanel& panel) override;

        void ApplyPanelTitle(GuiPanel& panel);

        PanelConfig config_{};
        std::shared_ptr<SharedState> state_{};
        String cachedDisplayName_{};
        String stablePanelId_{};
    };
}

#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Containers/String.h"
#include "Gui/Controllers/GuiPanelController.h"
#include "Gui/Core/GuiCommon.h"
#include "imgui.h"

namespace BixEngine::Gui
{
    class BaseAssetEditorController : public GuiPanelController
    {
    public:
        struct SharedState
        {
            struct VariableMetadata
            {
                String name{};
                String type{};
                String value{};
            };

            std::filesystem::path assetPath{};
            String assetDisplayName{};
            String stableIdRoot{};
            String assetTypeLabel{};
            std::function<void()> onCloseRequest{};
            std::string primaryClassName{};
            std::string includePath{};
            std::vector<VariableMetadata> exposedVariables{};
        };

        struct PanelConfig
        {
            String titlePrefix{};
            DockSpaceRegion dockRegion{DockSpaceRegion::Center};
            ImGuiCond dockCondition{ImGuiCond_FirstUseEver};
            String stableIdSuffix{};
        };

        struct EditorPage
        {
            String label{};
            std::function<void(GuiPanel&)> drawCallback{};
        };

        using PageId = std::size_t;

        BaseAssetEditorController(std::shared_ptr<SharedState> sharedState, PanelConfig config);
        ~BaseAssetEditorController() override = default;

        [[nodiscard]] std::shared_ptr<SharedState> GetSharedState() const noexcept { return state_; }

    protected:
        void DrawStandardToolbar();

        virtual void DrawPanelContents(GuiPanel& panel);
        virtual void OnPlayRequested();
        virtual void OnSaveRequested();
        virtual void OnCompileRequested();

        [[nodiscard]] const PanelConfig& GetPanelConfig() const noexcept { return config_; }

        template <typename Callback>
        PageId AddPage(String label, Callback&& drawCallback)
        {
            pages_.push_back(EditorPage{std::move(label), std::forward<Callback>(drawCallback)});
            return pages_.size() - 1;
        }

        void ClearPages();
        void SetActivePage(PageId id);
        [[nodiscard]] std::optional<PageId> GetActivePage() const noexcept;
        [[nodiscard]] bool DrawEditorPages(GuiPanel& panel);

        virtual void OnPageChanged(PageId  , const EditorPage&  ) {}

    private:
        void OnAttach(GuiPanel& panel) override;
        void OnDetach(GuiPanel& panel) override;
        void OnDraw(GuiPanel& panel) override;

        void ApplyPanelTitle(GuiPanel& panel);

        PanelConfig config_{};
        std::shared_ptr<SharedState> state_{};
        String cachedDisplayName_{};
        String stablePanelId_{};

        std::vector<EditorPage> pages_{};
        PageId activePage_{0};
    };
}


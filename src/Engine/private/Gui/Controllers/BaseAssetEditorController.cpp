#include "Gui/Controllers/BaseAssetEditorController.h"

#include <utility>

#include "Gui/Panels/GuiPanel.h"
#include "Logger.h"
#include "imgui.h"

namespace BixEngine::Gui
{
    namespace
    {
        String BuildStablePanelId(const BaseAssetEditorController::SharedState& state,
                                  const BaseAssetEditorController::PanelConfig& config)
        {
            String identifier = state.stableIdRoot;
            if (identifier.IsEmpty())
                identifier = "AssetEditor";

            if (!config.stableIdSuffix.IsEmpty())
            {
                identifier += "_";
                identifier += config.stableIdSuffix;
            }

            return identifier;
        }
    }

    BaseAssetEditorController::BaseAssetEditorController(std::shared_ptr<SharedState> sharedState, PanelConfig config)
        : config_(std::move(config)), state_(std::move(sharedState))
    {
        if (state_)
        {
            cachedDisplayName_ = state_->assetDisplayName;
            stablePanelId_ = BuildStablePanelId(*state_, config_);
        }
    }

    void BaseAssetEditorController::OnAttach(GuiPanel& panel)
    {
        panel.SetClosable(true);
        panel.SetMovable(true);
        panel.SetResizable(true);

        ApplyPanelTitle(panel);
        panel.SetDockingPreference(config_.dockRegion, config_.dockCondition);

        panel.OnClose = [state = state_]()
        {
            if (state && state->onCloseRequest)
                state->onCloseRequest();
        };
    }

    void BaseAssetEditorController::OnDetach(GuiPanel& panel)
    {
        panel.OnClose = nullptr;
    }

    void BaseAssetEditorController::OnDraw(GuiPanel& panel)
    {
        if (state_ && cachedDisplayName_ != state_->assetDisplayName)
        {
            cachedDisplayName_ = state_->assetDisplayName;
            ApplyPanelTitle(panel);
        }

        if (!DrawEditorPages(panel))
            DrawPanelContents(panel);
    }

    void BaseAssetEditorController::ApplyPanelTitle(GuiPanel& panel)
    {
        String title = config_.titlePrefix;
        if (title.IsEmpty())
            title = state_ && !state_->assetTypeLabel.IsEmpty() ? state_->assetTypeLabel : String{"Asset"};

        title += " - ";
        if (state_ && !state_->assetDisplayName.IsEmpty())
            title += state_->assetDisplayName;
        else
            title += "Untitled";

        title += "###";
        if (!stablePanelId_.IsEmpty())
            title += stablePanelId_;
        else if (state_ && !state_->stableIdRoot.IsEmpty())
            title += state_->stableIdRoot;
        else
            title += "AssetEditor";

        panel.SetTitle(std::move(title));
    }

    void BaseAssetEditorController::DrawStandardToolbar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14.0f, 6.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 0.0f));

        if (ImGui::Button("Play"))
            OnPlayRequested();

        ImGui::SameLine();
        if (ImGui::Button("Save"))
            OnSaveRequested();

        ImGui::SameLine();
        if (ImGui::Button("Compile"))
            OnCompileRequested();

        ImGui::PopStyleVar(2);
    }

    void BaseAssetEditorController::DrawPanelContents(GuiPanel& panel)
    {
        (void)panel;
    }

    void BaseAssetEditorController::OnPlayRequested()
    {
        if (!state_)
            return;

        LOG_INFO(String{"[AssetEditor] ▶ Play requested for asset: "} + state_->assetDisplayName);
    }

    void BaseAssetEditorController::OnSaveRequested()
    {
        if (!state_)
            return;

        LOG_INFO(String{"[AssetEditor] 💾 Save requested for asset: "} + state_->assetDisplayName);
    }

    void BaseAssetEditorController::OnCompileRequested()
    {
        if (!state_)
            return;

        LOG_INFO(String{"[AssetEditor] 🧠 Compile requested for asset: "} + state_->assetDisplayName);
    }

    void BaseAssetEditorController::ClearPages()
    {
        pages_.clear();
        activePage_ = 0;
    }

    void BaseAssetEditorController::SetActivePage(PageId id)
    {
        if (id >= pages_.size())
            return;

        if (activePage_ != id)
        {
            activePage_ = id;
            OnPageChanged(id, pages_[id]);
        }
    }

    std::optional<BaseAssetEditorController::PageId> BaseAssetEditorController::GetActivePage() const noexcept
    {
        if (pages_.empty())
            return std::nullopt;
        return activePage_;
    }

    bool BaseAssetEditorController::DrawEditorPages(GuiPanel& panel)
    {
        (void)panel;
        if (pages_.empty())
            return false;

        bool pageDrawn = false;
        if (ImGui::BeginTabBar("AssetEditorPages"))
        {
            for (PageId index = 0; index < pages_.size(); ++index)
            {
                const auto& page = pages_[index];
                const bool isSelected = index == activePage_;
                const ImGuiTabItemFlags flags = isSelected ? ImGuiTabItemFlags_SetSelected : 0;

                if (ImGui::BeginTabItem(page.label.View().data(), nullptr, flags))
                {
                    if (!isSelected)
                    {
                        activePage_ = index;
                        OnPageChanged(index, page);
                    }

                    if (page.drawCallback)
                        page.drawCallback(panel);

                    ImGui::EndTabItem();
                    pageDrawn = true;
                }
            }

            ImGui::EndTabBar();
        }

        return pageDrawn;
    }
}

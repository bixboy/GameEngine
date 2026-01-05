#include "Gui/Controllers/BaseAssetEditorWindow.h"
#include "Gui/Panels/GuiPanel.h"
#include "Debug/Logger.h"


namespace BixEngine::Gui
{
    namespace
    {
        String BuildStablePanelId(const BaseAssetEditorWindow::SharedState& state, const BaseAssetEditorWindow::PanelConfig& config)
        {
            String identifier = state.stableIdRoot;
            
            if (identifier.empty())
                identifier = "AssetEditor";

            if (!config.stableIdSuffix.empty())
            {
                identifier += "_";
                identifier += config.stableIdSuffix;
            }
            
            return identifier;
        }
    }

    BaseAssetEditorWindow::BaseAssetEditorWindow(std::shared_ptr<SharedState> sharedState, PanelConfig config) 
        : config_(std::move(config)), state_(std::move(sharedState))
    {
        if (state_)
        {
            cachedDisplayName_ = state_->assetDisplayName;
            cachedDirtyState_ = state_->isDirty;
            stablePanelId_ = BuildStablePanelId(*state_, config_);
        }
    }

    void BaseAssetEditorWindow::OnAttach(GuiPanel& panel)
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

    void BaseAssetEditorWindow::OnDetach(GuiPanel& panel)
    {
        panel.OnClose = nullptr;
    }

    void BaseAssetEditorWindow::OnDraw(GuiPanel& panel)
    {
        if (state_)
        {
            bool needsUpdate = (cachedDisplayName_ != state_->assetDisplayName) || (cachedDirtyState_ != state_->isDirty);
            if (needsUpdate)
            {
                cachedDisplayName_ = state_->assetDisplayName;
                cachedDirtyState_ = state_->isDirty;
                ApplyPanelTitle(panel);
            }
        }

        DrawStandardToolbar();
        DrawPanelContents(panel);
    }

    void BaseAssetEditorWindow::ApplyPanelTitle(GuiPanel& panel)
    {
        String title = config_.titlePrefix;
        if (title.empty())
            title = state_ ? state_->assetTypeLabel : String{"Asset"};

        title += " - ";
        if (state_ && !state_->assetDisplayName.empty())
        {
            title += state_->assetDisplayName;
            
            if (state_->isDirty)
                title += "*";
        }
        else
        {
            title += "Untitled";
        }

        title += "###";
        
        if (!stablePanelId_.empty())
            title += stablePanelId_;
        else
            title += "AssetEditor_Generic";

        panel.SetTitle(std::move(title));
    }

    void BaseAssetEditorWindow::DrawStandardToolbar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
        
        if (ImGui::Button("Save"))
        {
            OnSaveRequested();
        }

        ImGui::SameLine();
        DrawToolbarExtensions();

        ImGui::PopStyleVar();
        ImGui::Separator();
    }

    void BaseAssetEditorWindow::OnSaveRequested()
    {
        if (state_)
        {
            LOG_INFO(String{"[AssetEditor] Save requested: "} + state_->assetDisplayName);
        }
    }
}
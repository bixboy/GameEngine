#include "Engine/Gui/Core/NavBar/GuiNavigationBar.h"
#include "Engine/Gui/Core/GuiModule.h"
#include "Engine/Gui/Core/GuiSystem.h"
#include "Engine/Gui/Core/GuiLayoutManager.h"
#include "Engine/Gui/Core/GuiPanel.h"
#include "imgui.h"

namespace BixEngine::Core
{
    namespace
    {
        constexpr float kNavigationBarHeight = 38.0f;
        constexpr std::string_view kSceneNavigationId{"scene"};
    }

    GuiNavigationBar::GuiNavigationBar(Gui::GuiSystem& guiSystem, Gui::GuiLayoutManager& layoutManager, GuiModule& owner) : guiSystem_(&guiSystem), layoutManager_(&layoutManager), owner_(&owner)
    {
    }

    void GuiNavigationBar::Render()
    {
        if (!guiSystem_ || !guiSystem_->IsInitialized())
            return;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        if (!viewport)
            return;

        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y));
        ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, kNavigationBarHeight));
        ImGui::SetNextWindowViewport(viewport->ID);

        const ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoCollapse;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{0.08f, 0.08f, 0.09f, 0.96f});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 6.0f));

        if (ImGui::Begin("EditorNavigationBar", nullptr, flags))
        {
            if (layoutManager_)
                owner_->activeLayout_ = layoutManager_->GetCurrentLayout();

            const float buttonHeight = kNavigationBarHeight - 16.0f;
            DrawSceneButton(buttonHeight);
            DrawActorEditorTabs(buttonHeight);
        }

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

    bool GuiNavigationBar::DrawNavigationButton(const std::string& label, bool isActive, float buttonHeight) const
    {
        if (isActive)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.30f, 0.30f, 0.34f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.34f, 0.34f, 0.38f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.28f, 0.28f, 0.32f, 1.0f});
        }

        const ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        const float paddingX = 32.0f;
        const ImVec2 size{textSize.x + paddingX, buttonHeight};
        const bool clicked = ImGui::Button(label.c_str(), size);

        if (isActive)
            ImGui::PopStyleColor(3);

        return clicked;
    }

    bool GuiNavigationBar::DrawCloseButton(std::string_view label, float buttonHeight) const
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 4.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.42f, 0.12f, 0.12f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.58f, 0.16f, 0.16f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.36f, 0.10f, 0.10f, 1.0f});

        const float closeButtonSize = buttonHeight - 12.0f;
        const ImVec2 closeSize{std::max(12.0f, closeButtonSize), buttonHeight};
        const bool closeRequested = ImGui::Button("x", closeSize);

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Close %.*s", static_cast<int>(label.size()), label.data());

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        return closeRequested;
    }

    void GuiNavigationBar::DrawSceneButton(float buttonHeight)
    {
        static const std::string kSceneLabel{"Scene"};
        const bool sceneActive = owner_->activeNavigationId_ == kSceneNavigationId || owner_->actorEditors_.empty();

        if (!DrawNavigationButton(kSceneLabel, sceneActive, buttonHeight))
            return;

        owner_->activeNavigationId_ = std::string{kSceneNavigationId};
        owner_->activeLayout_ = Gui::EditorLayoutType::Scene;

        if (layoutManager_)
            layoutManager_->Switch(Gui::EditorLayoutType::Scene);

        owner_->RefreshActorPanelsVisibility();
        owner_->FocusSceneViewport();
    }

    void GuiNavigationBar::DrawActorEditorTabs(float buttonHeight)
    {
        if (owner_->actorEditorOrder_.empty())
            return;

        std::vector<std::string> closeRequests;
        std::vector<std::string> staleEntries;
        closeRequests.reserve(owner_->actorEditorOrder_.size());
        staleEntries.reserve(owner_->actorEditorOrder_.size());

        for (const std::string& navId : owner_->actorEditorOrder_)
        {
            auto entryIt = owner_->actorEditors_.find(navId);
            if (entryIt == owner_->actorEditors_.end())
            {
                staleEntries.push_back(navId);
                continue;
            }

            auto& entry = entryIt->second;
            if (entry.sharedState)
            {
                const std::string displayName = entry.sharedState->assetDisplayName.Std();
                if (entry.buttonLabel != displayName)
                    entry.buttonLabel = displayName;
            }

            ImGui::SameLine();
            const bool isActive = owner_->activeNavigationId_ == entry.navigationId;

            ImGui::PushID(entry.navigationId.c_str());

            const std::string& label = entry.buttonLabel.empty() ? entry.navigationId : entry.buttonLabel;
            if (DrawNavigationButton(label, isActive, buttonHeight))
            {
                owner_->activeNavigationId_ = entry.navigationId;
                owner_->activeLayout_ = Gui::EditorLayoutType::ActorEditor;
                if (layoutManager_)
                {
                    owner_->ApplyActorEditorPanels(entry);
                    layoutManager_->Switch(Gui::EditorLayoutType::ActorEditor);
                }

                owner_->RefreshActorPanelsVisibility();
                if (entry.panels.viewport)
                    owner_->focusRequests_.push_back(entry.panels.viewport->GetTitle().Std());
            }

            ImGui::SameLine(0.0f, 6.0f);
            if (DrawCloseButton(label, buttonHeight))
                closeRequests.push_back(entry.navigationId);

            ImGui::PopID();
        }

        for (const std::string& stale : staleEntries)
        {
            auto orderIt = std::find(owner_->actorEditorOrder_.begin(), owner_->actorEditorOrder_.end(), stale);
            if (orderIt != owner_->actorEditorOrder_.end())
                owner_->actorEditorOrder_.erase(orderIt);
        }

        for (const std::string& navigationId : closeRequests)
            owner_->CloseActorEditor(navigationId);
    }
}

#include "Engine/Gui/Core/NavBar/GuiNavigationBar.h"
#include "Engine/Gui/Core/GuiModule.h"
#include "Engine/Gui/Core/NavBar/GuiActorEditorManager.h"
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
            {
                if (auto* manager = owner_->GetActorEditorManager())
                    manager->OnLayoutChanged(layoutManager_->GetCurrentLayout());
            }

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
        auto* manager = owner_->GetActorEditorManager();
        const bool sceneActive = !manager || manager->GetActiveNavigationId() == kSceneNavigationId || !manager->HasEditors();

        if (!DrawNavigationButton(kSceneLabel, sceneActive, buttonHeight))
            return;

        if (manager)
            manager->ActivateScene(true);
        else
            owner_->FocusSceneViewport();
    }

    void GuiNavigationBar::DrawActorEditorTabs(float buttonHeight)
    {
        auto* manager = owner_->GetActorEditorManager();
        if (!manager || manager->GetEditorOrder().empty())
            return;

        std::vector<std::string> closeRequests;
        std::vector<std::string> staleEntries;
        closeRequests.reserve(manager->GetEditorOrder().size());
        staleEntries.reserve(manager->GetEditorOrder().size());

        for (const std::string& navId : manager->GetEditorOrder())
        {
            auto* entry = manager->FindEditor(navId);
            if (!entry)
            {
                staleEntries.push_back(navId);
                continue;
            }

            if (entry->sharedState)
            {
                const std::string displayName = entry->sharedState->assetDisplayName.Std();
                if (entry->buttonLabel != displayName)
                    entry->buttonLabel = displayName;
            }

            ImGui::SameLine();
            const bool isActive = manager->GetActiveNavigationId() == entry->navigationId;

            ImGui::PushID(entry->navigationId.c_str());

            const std::string& label = entry->buttonLabel.empty() ? entry->navigationId : entry->buttonLabel;
            if (DrawNavigationButton(label, isActive, buttonHeight))
                manager->ActivateEditor(entry->navigationId, true);

            ImGui::SameLine(0.0f, 6.0f);
            if (DrawCloseButton(label, buttonHeight))
                closeRequests.push_back(entry->navigationId);

            ImGui::PopID();
        }

        for (const std::string& stale : staleEntries)
            manager->RemoveNavigationIdFromOrder(stale);

        for (const std::string& navigationId : closeRequests)
            manager->CloseActorEditor(navigationId);
    }

    }
}

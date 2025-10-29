#include "Engine/Gui/Panels/ActorEditorPanel.h"

#include <algorithm>
#include <utility>

#include "imgui.h"

namespace BixEngine::Gui
{
    namespace
    {
        constexpr float kMinimumOutlineWidth = 220.0f;
        constexpr float kMinimumInspectorWidth = 260.0f;
        constexpr float kToolbarHeight = 36.0f;
        constexpr float kColumnSpacing = 8.0f;

        ImVec2 ComputeChildSize(float availableWidth, float ratio, float minimum) noexcept
        {
            const float width = std::max(availableWidth * ratio, minimum);
            return ImVec2{width, 0.0f};
        }
    }

    ActorEditorPanel::ActorEditorPanel(String name, String title)
        : GuiPanel(std::move(name), std::move(title))
    {
        SetCollapsable(false);
        SetClosable(true);
        SetResizable(true);
        SetMovable(true);
        AddWindowFlags(ImGuiWindowFlags_MenuBar);
        SetBackgroundColor(ImVec4{0.11f, 0.11f, 0.12f, 1.0f});
    }

    void ActorEditorPanel::DrawEditor()
    {
        DrawToolbar_();
        DrawLayout_();
    }

    void ActorEditorPanel::DrawToolbar_()
    {
        if (!ImGui::BeginChild("ActorEditorToolbar", ImVec2(0.0f, kToolbarHeight), false))
        {
            ImGui::EndChild();
            return;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14.0f, 6.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 0.0f));

        if (ImGui::Button("▶ Play"))
        {
            if (toolbar_.onPlay)
                toolbar_.onPlay();
        }

        ImGui::SameLine();
        if (ImGui::Button("💾 Save"))
        {
            if (toolbar_.onSave)
                toolbar_.onSave();
        }

        ImGui::SameLine();
        if (ImGui::Button("🧠 Compile"))
        {
            if (toolbar_.onCompile)
                toolbar_.onCompile();
        }

        ImGui::PopStyleVar(2);
        ImGui::EndChild();
    }

    void ActorEditorPanel::DrawLayout_()
    {
        const ImVec2 available = ImGui::GetContentRegionAvail();
        if (available.y <= 0.0f)
            return;

        const float viewportWidth = std::max(available.x - (available.x * outlineWidthRatio_) - (available.x * inspectorWidthRatio_) - (2.0f * kColumnSpacing), 320.0f);
        const ImVec2 viewportSize{viewportWidth, available.y};

        const ImVec2 outlineSize = ComputeChildSize(available.x, outlineWidthRatio_, kMinimumOutlineWidth);
        const ImVec2 inspectorSize = ComputeChildSize(available.x, inspectorWidthRatio_, kMinimumInspectorWidth);

        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));

        if (ImGui::BeginChild("ActorEditorViewport", viewportSize, true))
            DrawViewportArea_();
        ImGui::EndChild();

        ImGui::SameLine(0.0f, kColumnSpacing);

        if (ImGui::BeginChild("ActorEditorOutline", outlineSize, true))
            DrawOutlineArea_();
        ImGui::EndChild();

        ImGui::SameLine(0.0f, kColumnSpacing);

        if (ImGui::BeginChild("ActorEditorInspector", inspectorSize, true))
            DrawInspectorArea_();
        ImGui::EndChild();

        ImGui::PopStyleVar(2);
    }

    void ActorEditorPanel::DrawViewportArea_()
    {
        if (callbacks_.viewport)
        {
            callbacks_.viewport();
        }
        else
        {
            ImGui::TextUnformatted("Viewport preview unavailable.");
        }
    }

    void ActorEditorPanel::DrawOutlineArea_()
    {
        if (callbacks_.outline)
        {
            callbacks_.outline();
        }
        else
        {
            ImGui::TextUnformatted("No outline available.");
        }
    }

    void ActorEditorPanel::DrawInspectorArea_()
    {
        if (callbacks_.inspector)
        {
            callbacks_.inspector();
        }
        else
        {
            ImGui::TextUnformatted("Inspector not available.");
        }
    }
}

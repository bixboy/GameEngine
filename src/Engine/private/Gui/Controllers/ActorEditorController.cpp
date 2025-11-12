#include "Gui/Controllers/ActorEditorController.h"
#include <algorithm>
#include <utility>
#include "Logger.h"
#include "Gui/Internal/GuiPanel.h"
#include "Gui/Utils/GuiHelpers.h"
#include "imgui.h"
#include "Gui/GuiDocking.h"
#include "Gui/Controllers/BaseAssetEditorController.h"

namespace BixEngine::Gui
{
    namespace
    {
        String BuildDisplayName(const std::filesystem::path& path)
        {
            if (path.empty())
                return "Actor Prefab";

            const auto filename = path.filename().generic_string();
            if (!filename.empty())
                return filename;

            const auto stem = path.stem().generic_string();
            if (!stem.empty())
                return stem;

            return String(path.generic_string());
        }

        BaseAssetEditorController::PanelConfig MakePanelConfig(ActorEditorController::Section section)
        {
            using Section = ActorEditorController::Section;
            BaseAssetEditorController::PanelConfig config{};

            switch (section)
            {
            case Section::Toolbar:
                config.titlePrefix = "Actor Toolbar";
                config.dockRegion = DockSpaceRegion::Top;
                config.stableIdSuffix = "Toolbar";
                break;
            case Section::Viewport:
                config.titlePrefix = "Actor Viewport";
                config.dockRegion = DockSpaceRegion::Center;
                config.stableIdSuffix = "Viewport";
                break;
            case Section::Outline:
                config.titlePrefix = "Actor Outline";
                config.dockRegion = DockSpaceRegion::Left;
                config.stableIdSuffix = "Outline";
                break;
            case Section::Inspector:
                config.titlePrefix = "Actor Inspector";
                config.dockRegion = DockSpaceRegion::Right;
                config.stableIdSuffix = "Inspector";
                break;
            }

            return config;
        }
    }

    ActorEditorController::ActorEditorController(std::shared_ptr<SharedState> sharedState, Section section)
        : BaseAssetEditorController(std::move(sharedState), MakePanelConfig(section)), section_(section)
    {
    }

    std::shared_ptr<ActorEditorController::SharedState> ActorEditorController::CreateSharedState(
        std::filesystem::path assetPath, String stableIdRoot, std::function<void()> onCloseRequest)
    {
        auto state = std::make_shared<SharedState>();
        state->assetPath = std::move(assetPath);
        state->stableIdRoot = std::move(stableIdRoot);
        state->onCloseRequest = std::move(onCloseRequest);
        state->assetDisplayName = BuildDisplayName(state->assetPath);
        state->assetTypeLabel = "Actor Prefab";
        return state;
    }

    void ActorEditorController::DrawPanelContents(GuiPanel& panel)
    {
        static_cast<void>(panel);

        switch (section_)
        {
        case Section::Toolbar:
            DrawToolbar();
            break;
        case Section::Viewport:
            DrawViewport();
            break;
        case Section::Outline:
            DrawOutline();
            break;
        case Section::Inspector:
            DrawInspector();
            break;
        }
    }

    void ActorEditorController::OnPlayRequested()
    {
        if (const auto state = GetSharedState())
            LOG_INFO(String{"[ActorPrefabEditor] ▶ Play requested for asset: "} + state->assetDisplayName);
    }

    void ActorEditorController::OnSaveRequested()
    {
        if (const auto state = GetSharedState())
            LOG_INFO(String{"[ActorPrefabEditor] 💾 Save requested for asset: "} + state->assetDisplayName);
    }

    void ActorEditorController::OnCompileRequested()
    {
        if (const auto state = GetSharedState())
            LOG_INFO(String{"[ActorPrefabEditor] 🧠 Compile requested for asset: "} + state->assetDisplayName);
    }

    void ActorEditorController::DrawToolbar()
    {
        DrawStandardToolbar();
    }

    void ActorEditorController::DrawViewport()
    {
        const auto state = GetSharedState();
        ImVec2 available = ImGui::GetContentRegionAvail();
        if (available.x <= 0.0f || available.y <= 0.0f)
        {
            ImGui::TextUnformatted("Viewport unavailable.");
            return;
        }

        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        DrawViewportGrid_(available);
        ImGui::Dummy(available);

        ImGui::SetCursorScreenPos(ImVec2{cursor.x + 16.0f, cursor.y + 16.0f});
        ImGui::BeginGroup();
        ImGui::TextUnformatted("Prefab Preview");
        ImGui::Separator();
        if (state)
        {
            ImGui::Text("Asset: %s", state->assetDisplayName.View().data());
            if (!state->primaryClassName.empty())
                ImGui::Text("Script: %s", state->primaryClassName.c_str());
            if (!state->includePath.empty())
                ImGui::Text("Include: %s", state->includePath.c_str());
        }
        else
        {
            ImGui::TextUnformatted("No asset loaded.");
        }
        ImGui::EndGroup();
    }

    void ActorEditorController::DrawOutline()
    {
        const auto state = GetSharedState();
        if (!state)
        {
            Utils::DrawEmptyStateMessage("No asset metadata available.");
            return;
        }

        ImGui::TextUnformatted("Overview");
        ImGui::Separator();
        ImGui::BulletText("Type: %s", state->assetTypeLabel.IsEmpty()
                                          ? "Actor Prefab"
                                          : state->assetTypeLabel.View().data());
        if (!state->primaryClassName.empty())
            ImGui::BulletText("Script class: %s", state->primaryClassName.c_str());
        else
            ImGui::BulletText("Script class: <unknown>");

        if (!state->includePath.empty())
            ImGui::BulletText("Include: %s", state->includePath.c_str());
    }

    void ActorEditorController::DrawInspector()
    {
        const auto state = GetSharedState();
        if (!state)
        {
            Utils::DrawEmptyStateMessage("No asset selected.");
            return;
        }

        Utils::DrawDescriptionText("Inspect the metadata associated with this prefab.");
        ImGui::Spacing();

        Utils::DrawLabelValue("Name", state->assetDisplayName.View().data(), "Prefab");
        Utils::DrawLabelValue("Path", state->assetPath.generic_string(), "");
        Utils::DrawLabelValue("Script", state->primaryClassName.empty() ? "Unknown" : state->primaryClassName.c_str(),
                              "Unknown");
        if (!state->includePath.empty())
            Utils::DrawLabelValue("Include", state->includePath.c_str(), "");
    }

    void ActorEditorController::DrawViewportGrid_(const ImVec2& size)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const ImU32 gridColor = ImGui::GetColorU32(ImVec4{0.2f, 0.2f, 0.2f, 1.0f});
        const ImU32 axisColor = ImGui::GetColorU32(ImVec4{0.8f, 0.3f, 0.3f, 1.0f});

        constexpr float spacing = 24.0f;
        for (float x = 0.0f; x <= size.x; x += spacing)
        {
            const ImVec2 start{origin.x + x, origin.y};
            const ImVec2 end{origin.x + x, origin.y + size.y};
            drawList->AddLine(start, end, gridColor, (x == 0.0f) ? 2.0f : 1.0f);
        }

        for (float y = 0.0f; y <= size.y; y += spacing)
        {
            const ImVec2 start{origin.x, origin.y + y};
            const ImVec2 end{origin.x + size.x, origin.y + y};
            drawList->AddLine(start, end, gridColor, (y == 0.0f) ? 2.0f : 1.0f);
        }

        drawList->AddRect(origin, ImVec2{origin.x + size.x, origin.y + size.y}, axisColor, 0.0f, 0, 2.0f);
    }
}

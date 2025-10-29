#include "Engine/Gui/Controllers/ActorEditorController.h"

#include <algorithm>
#include <string>
#include <utility>

#include "Core/Logger.h"
#include "Engine/Gui/Core/GuiPanel.h"
#include "Engine/Systems/SubsystemManager.h"
#include "Game/Actor.h"
#include "Game/Components/Component.h"
#include "Game/Scene.h"
#include "Game/SceneManager.h"
#include "imgui.h"

#include "Engine/Gui/Panels/ActorInspector/ActorOverviewUI.h"
#include "Engine/Gui/Panels/ActorInspector/ComponentSectionUI.h"
#include "Engine/Gui/Panels/ActorInspector/TransformSectionUI.h"
#include "Engine/Gui/Panels/ActorInspector/Utils/ActorInspectorHelpers.h"
#include "Engine/Gui/Utils/GuiHelpers.h"

namespace BixEngine::Gui
{
    namespace
    {
        String BuildDisplayName(const std::filesystem::path& path)
        {
            if (path.empty())
                return "Actor";

            const auto filename = path.filename().generic_string();
            if (!filename.empty())
                return filename;

            return String(path.generic_string());
        }

        Game::Actor* ResolveActor(Core::SubsystemManager* subsystems, const std::filesystem::path& assetPath) noexcept
        {
            if (!subsystems)
                return nullptr;

            Game::Scene* scene = subsystems->GetScene();
            if (!scene)
                return nullptr;

            if (!assetPath.empty())
            {
                const String pathString(assetPath.generic_string());
                if (Game::Actor* resolved = scene->FindActorByPath(pathString))
                    return resolved;
            }

            return nullptr;
        }

        void DrawCenteredText(const char* text, float verticalOffset = 0.0f)
        {
            if (!text)
                return;

            const ImVec2 region = ImGui::GetContentRegionAvail();
            const ImVec2 textSize = ImGui::CalcTextSize(text);
            ImVec2 cursor = ImGui::GetCursorScreenPos();
            cursor.x += std::max(0.0f, (region.x - textSize.x) * 0.5f);
            cursor.y += std::max(0.0f, (region.y - textSize.y) * 0.5f) + verticalOffset;
            ImGui::GetWindowDrawList()->AddText(cursor, ImGui::GetColorU32(ImGuiCol_TextDisabled), text);
        }

        const char* SectionStableId(ActorEditorController::Section section)
        {
            switch (section)
            {
            case ActorEditorController::Section::Toolbar:
                return "ActorEditorToolbar";
            case ActorEditorController::Section::Viewport:
                return "ActorEditorViewport";
            case ActorEditorController::Section::Outline:
                return "ActorEditorOutline";
            case ActorEditorController::Section::Inspector:
                return "ActorEditorInspector";
            }

            return "ActorEditorPanel";
        }

        const char* SectionDisplayPrefix(ActorEditorController::Section section)
        {
            switch (section)
            {
            case ActorEditorController::Section::Toolbar:
                return "Actor Toolbar - ";
            case ActorEditorController::Section::Viewport:
                return "Actor Viewport - ";
            case ActorEditorController::Section::Outline:
                return "Actor Outline - ";
            case ActorEditorController::Section::Inspector:
                return "Actor Inspector - ";
            }

            return "Actor Editor - ";
        }
    }

    ActorEditorController::ActorEditorController(std::shared_ptr<SharedState> sharedState,
                                                 Section section)
        : section_(section)
        , state_(std::move(sharedState))
    {
    }

    std::shared_ptr<ActorEditorController::SharedState> ActorEditorController::CreateSharedState(Core::SubsystemManager& subsystems,
                                                                                                  std::filesystem::path assetPath,
                                                                                                  CloseRequest onCloseRequest)
    {
        auto state = std::make_shared<SharedState>();
        state->subsystems = &subsystems;
        state->assetPath = std::move(assetPath);
        state->assetDisplayName = BuildDisplayName(state->assetPath);
        state->onCloseRequest = std::move(onCloseRequest);
        state->actor = nullptr;
        state->actorRefreshRequested = true;
        return state;
    }

    void ActorEditorController::RequestActorReload()
    {
        if (state_)
            state_->actorRefreshRequested = true;
    }

    void ActorEditorController::OnAttach(GuiPanel& panel)
    {
        if (!state_)
            return;

        panel.SetClosable(true);
        panel.SetMovable(true);
        panel.SetResizable(true);

        cachedDisplayName_ = state_->assetDisplayName;
        ApplyPanelTitle(panel);

        switch (section_)
        {
        case Section::Toolbar:
            panel.SetDockingPreference(DockSpaceRegion::Top, ImGuiCond_FirstUseEver);
            break;
        case Section::Viewport:
            panel.SetDockingPreference(DockSpaceRegion::Center, ImGuiCond_FirstUseEver);
            break;
        case Section::Outline:
            panel.SetDockingPreference(DockSpaceRegion::Left, ImGuiCond_FirstUseEver);
            break;
        case Section::Inspector:
            panel.SetDockingPreference(DockSpaceRegion::Right, ImGuiCond_FirstUseEver);
            break;
        }

        panel.OnClose = [request = state_->onCloseRequest]()
        {
            if (request)
                request();
        };

        EnsureActorUpToDate();
    }

    void ActorEditorController::OnDetach(GuiPanel& panel)
    {
        panel.OnClose = nullptr;
    }

    void ActorEditorController::OnDraw(GuiPanel& panel)
    {
        static_cast<void>(panel);
        EnsureActorUpToDate();

        if (state_ && cachedDisplayName_ != state_->assetDisplayName)
        {
            cachedDisplayName_ = state_->assetDisplayName;
            ApplyPanelTitle(panel);
        }

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

    void ActorEditorController::ApplyPanelTitle(GuiPanel& panel)
    {
        const char* stableId = SectionStableId(section_);
        String title(SectionDisplayPrefix(section_));

        if (!cachedDisplayName_.IsEmpty())
            title += cachedDisplayName_;
        else if (state_ && !state_->assetDisplayName.IsEmpty())
            title += state_->assetDisplayName;
        else
            title += "Actor";

        title += "###";
        title += stableId;

        panel.SetTitle(std::move(title));
    }

    void ActorEditorController::EnsureActorUpToDate()
    {
        if (!state_ || !state_->actorRefreshRequested)
            return;

        state_->actor = ResolveActor(state_->subsystems, state_->assetPath);
        state_->actorRefreshRequested = false;

        if (state_->actor)
        {
            const String actorName = state_->actor->GetName();
            if (!actorName.IsEmpty() && state_->assetDisplayName != actorName)
                state_->assetDisplayName = actorName;
        }
    }

    void ActorEditorController::DrawViewport()
    {
        if (!state_)
            return;

        ImVec2 available = ImGui::GetContentRegionAvail();
        if (available.x <= 0.0f || available.y <= 0.0f)
        {
            ImGui::TextUnformatted("Viewport unavailable.");
            return;
        }

        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        DrawViewportGrid_(available);

        if (!state_->actor)
        {
            DrawCenteredText("Actor not found.");
            ImGui::Dummy(available);
            return;
        }

        ImGui::Dummy(available);

        ImGui::SetCursorScreenPos(ImVec2{cursor.x + 16.0f, cursor.y + 16.0f});
        ImGui::BeginGroup();
        const auto nameView = state_->actor->GetName().View();
        ImGui::Text("%s", state_->assetDisplayName.View().data());
        ImGui::Separator();
        ImGui::Text("Actor name: %.*s", static_cast<int>(nameView.size()), nameView.data());
        ImGui::Text("Components: %zu", state_->actor->GetComponents().size());
        ImGui::EndGroup();
    }

    void ActorEditorController::DrawOutline()
    {
        if (!state_)
            return;

        if (!state_->actor)
        {
            Utils::DrawEmptyStateMessage("No actor loaded.");
            return;
        }

        ImGui::TextUnformatted("Components");
        ImGui::Separator();

        auto& components = state_->actor->GetComponents();
        if (components.empty())
        {
            Utils::DrawEmptyStateMessage("Actor has no components.");
            return;
        }

        for (std::size_t index = 0; index < components.size(); ++index)
        {
            const auto& component = components[index];
            if (!component)
                continue;

            const std::string label = std::to_string(index + 1) + ". " + component->GetTypeName().Std();
            ImGui::BulletText("%s", label.c_str());
        }
    }

    void ActorEditorController::DrawInspector()
    {
        if (!state_)
            return;

        if (!state_->actor)
        {
            Utils::DrawEmptyStateMessage("No actor loaded.");
            return;
        }

        ActorInspector::DrawGeneralSection(*state_->actor);
        ActorInspector::DrawTransformSection(*state_->actor);
        ActorInspector::DrawComponentSection(*state_->actor);
    }

    void ActorEditorController::HandlePlayRequest()
    {
        if (!state_)
            return;

        LOG_INFO(String{"[ActorEditor] ▶ Play requested for asset: "} + state_->assetDisplayName);
    }

    void ActorEditorController::HandleSaveRequest()
    {
        if (!state_)
            return;

        LOG_INFO(String{"[ActorEditor] 💾 Save requested for asset: "} + state_->assetDisplayName);
    }

    void ActorEditorController::HandleCompileRequest()
    {
        if (!state_)
            return;

        LOG_INFO(String{"[ActorEditor] 🧠 Compile requested for asset: "} + state_->assetDisplayName);
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

    void ActorEditorController::DrawToolbar()
    {
        if (!state_)
            return;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14.0f, 6.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 0.0f));

        if (ImGui::Button("▶ Play"))
        {
            HandlePlayRequest();
        }

        ImGui::SameLine();
        if (ImGui::Button("💾 Save"))
        {
            HandleSaveRequest();
        }

        ImGui::SameLine();
        if (ImGui::Button("🧠 Compile"))
        {
            HandleCompileRequest();
        }

        ImGui::PopStyleVar(2);
    }
}

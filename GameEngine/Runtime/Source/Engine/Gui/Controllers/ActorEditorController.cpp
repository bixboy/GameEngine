#include "Engine/Gui/Controllers/ActorEditorController.h"

#include <algorithm>
#include <string>
#include <utility>

#include "Core/Logger.h"
#include "Engine/Gui/Panels/ActorEditorPanel.h"
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
    }

    ActorEditorController::ActorEditorController(Core::SubsystemManager& subsystems,
                                                 std::filesystem::path assetPath,
                                                 CloseRequest onCloseRequest)
        : subsystems_(&subsystems)
        , assetPath_(std::move(assetPath))
        , assetDisplayName_(BuildDisplayName(assetPath_))
        , onCloseRequest_(std::move(onCloseRequest))
    {
    }

    void ActorEditorController::RequestActorReload()
    {
        actorRefreshRequested_ = true;
    }

    void ActorEditorController::OnAttach(GuiPanel& panel)
    {
        if (auto* actorPanel = GetActorPanel(panel))
        {
            actorPanel->SetDrawCallbacks({
                [this]() { DrawViewport(); },
                [this]() { DrawOutline(); },
                [this]() { DrawInspector(); }
            });

            actorPanel->SetToolbarCallbacks({
                [this]() { HandlePlayRequest(); },
                [this]() { HandleSaveRequest(); },
                [this]() { HandleCompileRequest(); }
            });
        }

        panel.SetDockingPreference(DockSpaceRegion::Center, ImGuiCond_Always);
        panel.SetTitle(String{"Actor Editor - "} + assetDisplayName_);
        panel.SetClosable(true);
        panel.SetMovable(true);
        panel.SetResizable(true);

        panel.OnClose = [request = onCloseRequest_]()
        {
            if (request)
                request();
        };

        EnsureActorUpToDate();
    }

    void ActorEditorController::OnDetach(GuiPanel& panel)
    {
        if (auto* actorPanel = GetActorPanel(panel))
        {
            actorPanel->SetDrawCallbacks({});
            actorPanel->SetToolbarCallbacks({});
        }

        panel.OnClose = nullptr;
        actor_ = nullptr;
    }

    void ActorEditorController::OnDraw(GuiPanel& panel)
    {
        auto* actorPanel = GetActorPanel(panel);
        if (!actorPanel)
        {
            ImGui::TextUnformatted("Actor editor unavailable: panel type mismatch.");
            return;
        }

        EnsureActorUpToDate();
        actorPanel->DrawEditor();
    }

    ActorEditorPanel* ActorEditorController::GetActorPanel(GuiPanel& panel) noexcept
    {
        return dynamic_cast<ActorEditorPanel*>(&panel);
    }

    Game::Actor* ActorEditorController::ResolveActor() noexcept
    {
        if (!subsystems_)
            return nullptr;

        Game::Scene* scene = subsystems_->GetScene();
        if (!scene)
            return nullptr;

        if (!assetPath_.empty())
        {
            const String pathString(assetPath_.generic_string());
            if (Game::Actor* resolved = scene->FindActorByPath(pathString))
                return resolved;
        }

        return nullptr;
    }

    void ActorEditorController::EnsureActorUpToDate()
    {
        if (!actorRefreshRequested_)
            return;

        actor_ = ResolveActor();
        actorRefreshRequested_ = false;
    }

    void ActorEditorController::DrawViewport()
    {
        ImVec2 available = ImGui::GetContentRegionAvail();
        if (available.x <= 0.0f || available.y <= 0.0f)
        {
            ImGui::TextUnformatted("Viewport unavailable.");
            return;
        }

        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        DrawViewportGrid_(available);

        if (!actor_)
        {
            DrawCenteredText("Actor not found.");
            ImGui::Dummy(available);
            return;
        }

        ImGui::Dummy(available);

        ImGui::SetCursorScreenPos(cursor + ImVec2{16.0f, 16.0f});
        ImGui::BeginGroup();
        const auto nameView = actor_->GetName().View();
        ImGui::Text("%s", assetDisplayName_.View().data());
        ImGui::Separator();
        ImGui::Text("Actor name: %.*s", static_cast<int>(nameView.size()), nameView.data());
        ImGui::Text("Components: %zu", actor_->GetComponents().size());
        ImGui::EndGroup();
    }

    void ActorEditorController::DrawOutline()
    {
        if (!actor_)
        {
            Utils::DrawEmptyStateMessage("No actor loaded.");
            return;
        }

        ImGui::TextUnformatted("Components");
        ImGui::Separator();

        auto& components = actor_->GetComponents();
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
        if (!actor_)
        {
            Utils::DrawEmptyStateMessage("No actor loaded.");
            return;
        }

        ActorInspector::DrawGeneralSection(*actor_);
        ActorInspector::DrawTransformSection(*actor_);
        ActorInspector::DrawComponentSection(*actor_);
    }

    void ActorEditorController::HandlePlayRequest()
    {
        LOG_INFO(String{"[ActorEditor] ▶ Play requested for asset: "} + assetDisplayName_);
    }

    void ActorEditorController::HandleSaveRequest()
    {
        LOG_INFO(String{"[ActorEditor] 💾 Save requested for asset: "} + assetDisplayName_);
    }

    void ActorEditorController::HandleCompileRequest()
    {
        LOG_INFO(String{"[ActorEditor] 🧠 Compile requested for asset: "} + assetDisplayName_);
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

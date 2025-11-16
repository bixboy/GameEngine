#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "Gui/Controllers/BaseAssetEditorController.h"
#include "Gui/GuiManager.h"
#include "imgui.h"

namespace BixEngine::Gui::Examples
{
    /**
     * @brief Exemple concret d'éditeur d'acteur utilisant les sous-pages.
     */
    class ActorPrefabEditor final : public BaseAssetEditorController
    {
    public:
        explicit ActorPrefabEditor(std::shared_ptr<SharedState> sharedState)
            : BaseAssetEditorController(std::move(sharedState), PanelConfig{.titlePrefix = "Actor Prefab",
                                                                            .dockRegion = DockSpaceRegion::Center,
                                                                            .dockCondition = ImGuiCond_FirstUseEver,
                                                                            .stableIdSuffix = "Main"})
        {
            AddPage("General", [this](GuiPanel& panel) { DrawGeneralPage(panel); });
            AddPage("Transform", [this](GuiPanel& panel) { DrawTransformPage(panel); });
            AddPage("Components", [this](GuiPanel& panel) { DrawComponentsPage(panel); });
            AddPage("Scripts", [this](GuiPanel& panel) { DrawScriptsPage(panel); });
            AddPage("Debug", [this](GuiPanel& panel) { DrawDebugPage(panel); });
        }

    protected:
        void DrawPanelContents(GuiPanel& panel) override
        {
            if (DrawEditorPages(panel))
                return;

            ImGui::TextUnformatted("Aucune page n'est disponible.");
        }

    private:
        void DrawGeneralPage(GuiPanel& panel)
        {
            (void)panel;
            if (const auto state = GetSharedState())
            {
                ImGui::Text("Nom: %s", state->assetDisplayName.View().data());
                ImGui::Text("Chemin: %s", state->assetPath.generic_string().c_str());
            }
            else
            {
                ImGui::TextUnformatted("Aucun asset chargé.");
            }
        }

        void DrawTransformPage(GuiPanel& panel)
        {
            (void)panel;
            ImGui::TextUnformatted("Transformations locales :");
            ImGui::Separator();
            ImGui::DragFloat3("Position", transform_.data(), 0.05f);
            ImGui::DragFloat3("Rotation", rotation_.data(), 0.1f);
            ImGui::DragFloat3("Scale", scale_.data(), 0.01f, 0.01f, 10.0f);
        }

        void DrawComponentsPage(GuiPanel& panel)
        {
            (void)panel;
            ImGui::TextUnformatted("Composants liés :");
            ImGui::Separator();
            for (const auto& component : components_)
                ImGui::BulletText("%s", component.c_str());
        }

        void DrawScriptsPage(GuiPanel& panel)
        {
            (void)panel;
            ImGui::TextUnformatted("Scripts attachés");
            ImGui::Separator();
            if (const auto state = GetSharedState())
            {
                if (!state->primaryClassName.empty())
                    ImGui::Text("Classe principale : %s", state->primaryClassName.c_str());
                else
                    ImGui::TextUnformatted("Aucune classe associée.");
            }
        }

        void DrawDebugPage(GuiPanel& panel)
        {
            (void)panel;
            ImGui::TextUnformatted("Outils de debug");
            ImGui::Separator();
            if (ImGui::Button("Forcer la recompilation"))
                OnCompileRequested();
            ImGui::SameLine();
            if (ImGui::Button("Recharger l'asset"))
                ImGui::OpenPopup("ReloadDialog");

            if (ImGui::BeginPopup("ReloadDialog"))
            {
                ImGui::TextUnformatted("Recharger l'asset depuis le disque ?");
                if (ImGui::Button("Oui"))
                    ImGui::CloseCurrentPopup();
                ImGui::SameLine();
                if (ImGui::Button("Annuler"))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
        }

        std::array<float, 3> transform_{0.0f, 1.0f, 0.0f};
        std::array<float, 3> rotation_{0.0f, 0.0f, 0.0f};
        std::array<float, 3> scale_{1.0f, 1.0f, 1.0f};
        std::vector<std::string> components_{"MeshRenderer", "CameraComponent", "ScriptRunner"};
    };

    inline void OpenActorEditor(GuiManager& guiManager, const std::filesystem::path& assetPath)
    {
        BaseAssetEditorController::PanelConfig config{};
        config.titlePrefix = "Actor Editor";
        config.dockRegion = DockSpaceRegion::Center;
        config.stableIdSuffix = "Actor";

        auto& registry = guiManager.GetAssetEditorRegistry();
        registry.OpenEditor<ActorPrefabEditor>(assetPath, config);
    }

    inline void ShowcaseNavigation(GuiManager& guiManager)
    {
        guiManager.NavigateBack();
        guiManager.NavigateForward();
        guiManager.NavigateHome();
    }

    inline void SetupWorkspaces(GuiManager& guiManager)
    {
        WorkspaceRegistry::Workspace editorWorkspace{};
        editorWorkspace.name = "Editor";
        editorWorkspace.homePanel = "Scene";
        editorWorkspace.onActivate = [](GuiManager& manager)
        {
            manager.FocusPanel("SceneViewport");
        };

        WorkspaceRegistry::Workspace actorWorkspace{};
        actorWorkspace.name = "ActorEditor";
        actorWorkspace.homePanel = "ActorEditor::Main";
        actorWorkspace.onActivate = [](GuiManager& manager)
        {
            manager.FocusPanel("ActorEditor::Main");
        };

        guiManager.RegisterWorkspace(std::move(editorWorkspace));
        guiManager.RegisterWorkspace(std::move(actorWorkspace));
    }
}


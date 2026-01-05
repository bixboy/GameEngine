#include "Gui/Internal/MainMenuBar.h"
#include "Gui/Core/GuiManager.h"
#include "Gui/Internal/GuiLayoutManager.h"
#include "Gui/Internal/EditorSceneManager.h"
#include "Gui/Core/EditorPreferences.h"
#include "imgui.h"


namespace BixEngine::Gui
{
    MainMenuBar::MainMenuBar(GuiManager& guiManager, GuiLayoutManager& layoutManager, EditorSceneManager& sceneManager)
        : guiManager_(guiManager), layoutManager_(layoutManager), sceneManager_(sceneManager)
    {
    }

    void MainMenuBar::Draw()
    {
        if (ImGui::BeginMainMenuBar())
        {
            DrawFileMenu_();
            DrawEditMenu_();
            DrawWindowsMenu_();
            ImGui::EndMainMenuBar();
        }

        EditorPreferencesWindow::Draw(&showEditorPreferences_);
    }

    void MainMenuBar::DrawFileMenu_()
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Scene"))
            {
                sceneManager_.RequestNewScene();
            }

            if (ImGui::MenuItem("Open Scene..."))
            {
                sceneManager_.RequestOpenScene();
            }

            if (ImGui::BeginMenu("Recent Scenes", !sceneManager_.GetRecentScenes().empty()))
            {
                const auto& recents = sceneManager_.GetRecentScenes();
                for (size_t i = 0; i < recents.size(); ++i)
                {
                    const auto& path = recents[i];
                    std::string label = path.filename().string();
                    if (label.empty())
                        label = "Unknown";
                    
                    std::string menuId = label + "##recent_" + std::to_string(i);
                    if (ImGui::MenuItem(menuId.c_str()))
                    {
                        sceneManager_.LoadScene(path);
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::Separator();

            bool hasScene = !sceneManager_.GetCurrentScenePath().empty();
            if (ImGui::MenuItem("Close Scene", nullptr, false, hasScene))
            {
                sceneManager_.RequestCloseScene();
            }

            if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
            {
                sceneManager_.RequestSaveScene();
            }

            if (ImGui::MenuItem("Save Scene As..."))
            {
                 sceneManager_.RequestSaveSceneAs();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Rename Scene...", nullptr, false, hasScene))
            {
                sceneManager_.RequestRenameScene();
            }

            if (ImGui::MenuItem("Delete Scene..."))
            {
                sceneManager_.RequestDeleteScene();
            }
            
            ImGui::Separator();

            if (!sceneManager_.GetCurrentScenePath().empty())
            {
                 ImGui::TextDisabled("Current: %s", sceneManager_.GetCurrentScenePath().stem().string().c_str());
                 if (ImGui::IsItemHovered())
                 {
                     ImGui::SetTooltip("%s", sceneManager_.GetCurrentScenePath().string().c_str());
                 }
            }
            else
            {
                 ImGui::TextDisabled("No scene loaded");
            }

            ImGui::EndMenu();
        }
    }

    void MainMenuBar::DrawEditMenu_()
    {
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Editor Preferences..."))
            {
                showEditorPreferences_ = true;
            }
            
            ImGui::EndMenu();
        }
    }

    void MainMenuBar::DrawWindowsMenu_()
    {
        if (ImGui::BeginMenu("Windows"))
        {
            for (GuiPanel* panel : guiManager_.GetPanels())
            {
                if (!panel) continue;
                
                if (!layoutManager_.IsPanelVisibleInCurrentLayout(panel))
                {
                     continue;
                }
                
                bool visible = panel->IsVisible();
                if (ImGui::MenuItem(panel->GetTitle().c_str(), nullptr, &visible))
                {
                    panel->SetVisible(visible);
                    if (visible)
                    {
                        layoutManager_.AddPanel(layoutManager_.GetCurrentLayout(), *panel);
                    }
                    else
                    {
                        layoutManager_.RemovePanel(*panel);
                    }
                }
            }
            ImGui::EndMenu();
        }
    }
}

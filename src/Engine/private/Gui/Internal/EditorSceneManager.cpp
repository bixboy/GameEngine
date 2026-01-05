#include "Gui/Internal/EditorSceneManager.h"
#include <filesystem>
#include "Debug/Logger.h"
#include "Utils/FileIO/FilesUtils.h"
#include "Game/SceneManager.h"
#include "Serializer/SceneSerializer.h"
#include "Levels/EmptyScene.h"
#include <fstream>
#include <algorithm>

#include "Debug/Logger.h"


namespace BixEngine::Gui
{
    EditorSceneManager::EditorSceneManager()
    {
        recentScenesFile_ = Utils::FileUtils::ResolveUserConfigPath("recent_scenes.txt");
        LoadRecentScenes_();
    }

    void EditorSceneManager::RequestNewScene()
    {
        if (auto* sceneManager = Game::SceneManager::GetActiveSceneManager())
        {
            sceneManager->SetScene(std::make_unique<Game::EmptyScene>());
            currentScenePath_.clear();
            isSceneDirty_ = false;
        }
    }

    void EditorSceneManager::RequestOpenScene()
    {
        showOpenSceneDialog_ = true;
    }

    void EditorSceneManager::RequestSaveScene()
    {
        if (currentScenePath_.empty())
        {
            RequestSaveSceneAs();
        }
        else
        {
            PerformSave_(currentScenePath_);
        }
    }

    void EditorSceneManager::RequestSaveSceneAs()
    {
        showSaveAsDialog_ = true;
        saveAsFilenameBuffer_[0] = '\0';
    }

    void EditorSceneManager::RequestCloseScene()
    {
        if (isSceneDirty_)
        {
            showCloseSceneConfirmation_ = true;
        }
        else
        {
             RequestNewScene();
        }
    }

    void EditorSceneManager::RequestRenameScene()
    {
        showRenameSceneDialog_ = true;
        std::string currentName = currentScenePath_.stem().string();
        strncpy_s(renameFilenameBuffer_, currentName.c_str(), sizeof(renameFilenameBuffer_) - 1);
    }

    void EditorSceneManager::RequestDeleteScene()
    {
        showDeleteSceneDialog_ = true;
    }

    void EditorSceneManager::PerformSave_(const std::filesystem::path& path)
    {
        if (auto* sceneManager = Game::SceneManager::GetActiveSceneManager())
        {
            if (auto* scene = sceneManager->GetScene())
            {
                Serialization::SceneSerializer::SaveBinary(*scene, path);
                isSceneDirty_ = false;
                AddToRecentScenes_(path);
                currentScenePath_ = path;
            }
        }
    }

    void EditorSceneManager::LoadScene(const std::filesystem::path& path)
    {
        PerformLoad_(path);
    }

    void EditorSceneManager::PerformLoad_(const std::filesystem::path& path)
    {
        if (std::filesystem::exists(path))
        {
             if (auto* sceneManager = Game::SceneManager::GetActiveSceneManager())
             {
                 auto newScene = std::make_unique<Game::EmptyScene>();
                 newScene->SetName(path.stem().string().c_str());
                 if (Serialization::SceneSerializer::LoadBinary(*newScene, path))
                 {
                     sceneManager->SetScene(std::move(newScene));
                     currentScenePath_ = path;
                     isSceneDirty_ = false;
                     AddToRecentScenes_(path);
                 }
             }
        }
    }

    void EditorSceneManager::DrawDialogs()
    {
        DrawSaveAsDialog_();
        DrawOpenSceneDialog_();
        DrawDeleteSceneDialog_();
        DrawRenameSceneDialog_();
        DrawCloseSceneConfirmation_();
    }

    void EditorSceneManager::DrawSaveAsDialog_()
    {
        if (showSaveAsDialog_)
        {
            ImGui::OpenPopup("Save Scene As");
            showSaveAsDialog_ = false;
        }

        if (ImGui::BeginPopupModal("Save Scene As", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::InputText("Filename", saveAsFilenameBuffer_, sizeof(saveAsFilenameBuffer_));
            
            if (ImGui::Button("Save", ImVec2(120, 0)))
            {
                std::string filename = saveAsFilenameBuffer_;
                if (!filename.empty())
                {
                    if (!filename.ends_with(".bix"))
                        filename += ".bix";
                    
                    std::filesystem::create_directories("assets/scenes");
                    std::filesystem::path fullPath = std::filesystem::path("assets/scenes") / filename;

                     if (auto* sceneManager = Game::SceneManager::GetActiveSceneManager())
                    {
                        if (auto* scene = sceneManager->GetScene())
                        {
                            scene->SetName(std::filesystem::path(filename).stem().string().c_str());
                        }
                    }
                    
                    PerformSave_(fullPath);
                    ImGui::CloseCurrentPopup();
                }
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
    }

    void EditorSceneManager::DrawOpenSceneDialog_()
    {
        if (showOpenSceneDialog_)
        {
            ImGui::OpenPopup("Open Scene");
            showOpenSceneDialog_ = false;
        }

        if (ImGui::BeginPopupModal("Open Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static std::vector<std::filesystem::path> sceneFiles;
            if (ImGui::IsWindowAppearing())
            {
                sceneFiles.clear();
                if (std::filesystem::exists("assets/scenes"))
                {
                    for (const auto& entry : std::filesystem::directory_iterator("assets/scenes"))
                    {
                        if (entry.path().extension() == ".bix")
                        {
                            sceneFiles.push_back(entry.path());
                        }
                    }
                }
            }

            if (sceneFiles.empty())
            {
                ImGui::Text("No scenes found in assets/scenes/");
            }
            else
            {
                for (size_t i = 0; i < sceneFiles.size(); ++i)
                {
                    const auto& path = sceneFiles[i];
                    std::string label = path.filename().string();
                    if (label.empty())
                        label = "Unknown";
                    
                    std::string selectId = label + "##open_" + std::to_string(i);
                    if (ImGui::Selectable(selectId.c_str()))
                    {
                        PerformLoad_(path);
                        ImGui::CloseCurrentPopup();
                    }
                }
            }

            ImGui::Separator();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void EditorSceneManager::DrawDeleteSceneDialog_()
    {
         if (showDeleteSceneDialog_)
        {
            ImGui::OpenPopup("Delete Scene");
            showDeleteSceneDialog_ = false;
        }

        if (ImGui::BeginPopupModal("Delete Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Not implemented yet.");
            
             if (ImGui::Button("Close", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
    }
    
    void EditorSceneManager::DrawRenameSceneDialog_()
    {
        if (showRenameSceneDialog_)
        {
            ImGui::OpenPopup("Rename Scene");
            showRenameSceneDialog_ = false;
            renameErrorMessage_.clear();
        }

         if (ImGui::BeginPopupModal("Rename Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::InputText("New Name", renameFilenameBuffer_, sizeof(renameFilenameBuffer_));

            if (!renameErrorMessage_.empty())
            {
                ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "%s", renameErrorMessage_.c_str());
            }

            if (ImGui::Button("Rename", ImVec2(120, 0)))
            {
                 bool success = true;
                 if (currentScenePath_.empty())
                 {
                      if (auto* sceneManager = Game::SceneManager::GetActiveSceneManager())
                      {
                          if (auto* scene = sceneManager->GetScene())
                              scene->SetName(renameFilenameBuffer_);
                      }
                 }
                 else
                 {
                     std::string newName = renameFilenameBuffer_;
                     if (!newName.empty())
                     {
                         if (!newName.ends_with(".bix"))
                            newName += ".bix";

                     std::filesystem::path newPath = currentScenePath_.parent_path() / newName;
                     
                     // Use FileUtils for safe rename
                     BixEngine::String errorMsg;
                     if (!Utils::FileUtils::TryRename(currentScenePath_, newPath, false, errorMsg))
                     {
                         renameErrorMessage_ = errorMsg.Std();
                         success = false;
                     }
                     else
                     {
                         // Update internal state on success
                         currentScenePath_ = newPath;
                         AddToRecentScenes_(newPath);

                         if (auto* sceneManager = Game::SceneManager::GetActiveSceneManager())
                         {
                             if (auto* scene = sceneManager->GetScene())
                                 scene->SetName(std::filesystem::path(newName).stem().string().c_str());
                         }
                     }
                     else
                     {
                         renameErrorMessage_ = "Name cannot be empty.";
                         success = false;
                     }
                 }
                
                if (success)
                    ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
             
            ImGui::EndPopup();
        }
    }

    void EditorSceneManager::DrawCloseSceneConfirmation_()
    {
        if (showCloseSceneConfirmation_)
        {
            ImGui::OpenPopup("Close Scene?");
            showCloseSceneConfirmation_ = false;
        }

        if (ImGui::BeginPopupModal("Close Scene?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("You have unsaved changes. Are you sure you want to close?");
            
            if (ImGui::Button("Yes, Close", ImVec2(120, 0)))
            {
                RequestNewScene();
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
    }

    void EditorSceneManager::AddToRecentScenes_(const std::filesystem::path& path)
    {
        auto it = std::remove(recentScenes_.begin(), recentScenes_.end(), path);
        if (it != recentScenes_.end())
            recentScenes_.erase(it, recentScenes_.end());

        recentScenes_.insert(recentScenes_.begin(), path);

        if (recentScenes_.size() > 10)
            recentScenes_.resize(10);

        SaveRecentScenes_();
    }

    void EditorSceneManager::LoadRecentScenes_()
    {
        if (recentScenesFile_.empty() || !std::filesystem::exists(recentScenesFile_))
            return;

        std::ifstream file(recentScenesFile_);
        if (!file.is_open())
            return;

        std::string line;
        while (std::getline(file, line))
        {
            if (!line.empty())
                recentScenes_.emplace_back(line);
        }
    }

    void EditorSceneManager::SaveRecentScenes_()
    {
        if (recentScenesFile_.empty())
            return;

        std::ofstream file(recentScenesFile_);
        if (!file.is_open())
            return;

        for (const auto& path : recentScenes_)
        {
            file << path.string() << "\n";
        }
    }
}

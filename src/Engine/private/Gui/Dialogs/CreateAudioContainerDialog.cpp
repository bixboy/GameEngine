#include "Gui/Dialogs/CreateAudioContainerDialog.h"
#include "Ressources/RessourcesClass/AudioContainer.h"
#include "Utils/FileIO/FilesUtils.h"
#include <imgui.h>

namespace BixEngine::Gui
{
    CreateAudioContainerDialog::CreateAudioContainerDialog(ContentBrowserState& state, String& selectedEntry)
        : ModalDialog(state, selectedEntry, "Create Audio Container")
    {
    }

    void CreateAudioContainerDialog::Open(const std::filesystem::path& currentPath)
    {
        currentPath_ = currentPath;
        memset(nameBuffer_, 0, sizeof(nameBuffer_));
        strcpy_s(nameBuffer_, "NewAudioContainer");
        error_.clear();
        ModalDialog::Open();
    }

    void CreateAudioContainerDialog::DrawContent()
    {
        ImGui::Text("Name:");
        if (ImGui::InputText("##Name", nameBuffer_, sizeof(nameBuffer_), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            
        }

        if (!error_.IsEmpty())
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", error_.c_str());
        }

        ImGui::Separator();

        if (ImGui::Button("Create"))
        {
            String name = nameBuffer_;
            if (name.empty())
            {
                error_ = "Name cannot be empty.";
            }
            else
            {
                if (!name.EndsWith(".bixaudio"))
                    name += ".bixaudio";

                std::filesystem::path fullPath = currentPath_ / name.c_str();

                if (std::filesystem::exists(fullPath))
                {
                    error_ = "File already exists.";
                }
                else
                {
                    
                    BixEngine::Resources::AudioContainer container;
                    if (container.SaveToFile(fullPath.string()))
                    {
                        Close();
                        state_.cache.dirty = true;
                    }
                    else
                    {
                        error_ = "Failed to save file.";
                    }
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            Close();
        }
    }
}

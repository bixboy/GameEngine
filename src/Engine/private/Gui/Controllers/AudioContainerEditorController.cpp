#include "Gui/Controllers/AudioContainerEditorController.h"
#include "Ressources/Core/ResourceManager.h"
#include "Gui/Widgets/Widgets.h"
#include "Utils/FileIO/FilesUtils.h"
#include <imgui.h>
#include <filesystem>
#include "Systems/Audio/AudioSystem.h"
#include <miniaudio.h>

namespace BixEngine::Gui
{
    struct AudioContainerEditorController::PreviewState
    {
        ma_sound sound;
        bool initialized = false;
        int trackIndex = -1;
    };

    AudioContainerEditorController::AudioContainerEditorController(std::shared_ptr<SharedState> sharedState)
        : BaseAssetEditorController(std::move(sharedState), PanelConfig{ .titlePrefix = "Audio Container", .dockRegion = DockSpaceRegion::Center })
    {
        previewState_ = std::make_unique<PreviewState>();
        LoadResource();
    }

    AudioContainerEditorController::~AudioContainerEditorController()
    {
        if (previewState_ && previewState_->initialized)
        {
            ma_sound_uninit(&previewState_->sound);
        }
    }

    namespace
    {
        std::filesystem::path DetermineAudioRoot()
        {
            std::error_code ec;
            const std::filesystem::path base = std::filesystem::current_path(ec);
            if (ec) return {};
            const std::filesystem::path content = base / "Content";
            if (std::filesystem::exists(content)) return content;
            const std::filesystem::path resources = base / "Resources";
            if (std::filesystem::exists(resources)) return resources;
            return {};
        }

        void CollectFiles(std::vector<std::filesystem::path>& outFiles, const std::vector<std::string>& extensions)
        {
            const std::filesystem::path root = DetermineAudioRoot();
            if (root.empty()) return;

            std::error_code ec;
            for (std::filesystem::recursive_directory_iterator it(root, ec), end; it != end; it.increment(ec))
            {
                if (ec) break;
                if (!it->is_regular_file()) continue;

                std::string ext = it->path().extension().generic_string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
                
                for (const auto& validExt : extensions)
                {
                    if (ext == validExt)
                    {
                        outFiles.push_back(it->path());
                        break;
                    }
                }
            }

            std::sort(outFiles.begin(), outFiles.end(),
              [](const std::filesystem::path& a, const std::filesystem::path& b)
              {
                  return a.generic_string() < b.generic_string();
              });
        }

        std::string MakeDisplayName(const std::filesystem::path& path, const std::filesystem::path& root)
        {
            if (root.empty()) return path.generic_string();
            std::error_code ec;
            const std::filesystem::path relative = std::filesystem::relative(path, root, ec);
            if (!ec && !relative.empty()) return relative.generic_string();
            return path.generic_string();
        }
    }

    void AudioContainerEditorController::LoadResource()
    {
        auto state = GetSharedState();
        if (!state) return;

        
        audioContainer_ = resources::ResourceManager::Get().Get<resources::AudioContainer>(state->assetPath.string());
        if (!audioContainer_)
        {
            
            audioContainer_ = std::make_shared<resources::AudioContainer>();
            if (!audioContainer_->LoadFromFile(state->assetPath.string()))
            {
                
            }
        }
    }

    void AudioContainerEditorController::OnSaveRequested()
    {
        if (audioContainer_ && GetSharedState())
        {
            if (audioContainer_->SaveToFile(GetSharedState()->assetPath.string()))
            {
                isDirty_ = false;
            }
        }
    }

    void AudioContainerEditorController::DrawPanelContents(GuiPanel& panel)
    {
        (void)panel;
        if (!audioContainer_)
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to load Audio Container.");
            return;
        }

        DrawStandardToolbar();
        
        
        if (ImGui::BeginChild("MainContent", ImVec2(0, 0), true))
        {
            ImGui::Spacing();
            
            
            if (ImGui::CollapsingHeader("Container Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Indent();
                DrawProperties();
                ImGui::Unindent();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            
            if (ImGui::CollapsingHeader("Audio Tracks", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Indent();
                DrawTracks();
                ImGui::Unindent();
            }
            
            ImGui::EndChild();
        }
    }

    void AudioContainerEditorController::DrawProperties()
    {
        bool changed = false;

        if (ImGui::BeginTable("PropertiesTable", 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("Value");
            
            
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Playback Mode");
            ImGui::TableSetColumnIndex(1);
            int mode = static_cast<int>(audioContainer_->Mode);
            if (ImGui::Combo("##Mode", &mode, "Random\0Sequence\0"))
            {
                audioContainer_->Mode = static_cast<resources::AudioContainerMode>(mode);
                changed = true;
            }

            
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Looping");
            ImGui::TableSetColumnIndex(1);
            if (ImGui::Checkbox("##Loop", &audioContainer_->Loop))
                changed = true;

            
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Volume Variance");
            ImGui::TableSetColumnIndex(1);
            if (ImGui::DragFloat("##VolVar", &audioContainer_->VolumeVariance, 0.01f, 0.0f, 1.0f, "%.2f"))
                changed = true;
            
            
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Pitch Variance");
            ImGui::TableSetColumnIndex(1);
            if (ImGui::DragFloat("##PitchVar", &audioContainer_->PitchVariance, 0.01f, 0.0f, 1.0f, "%.2f"))
                changed = true;

            ImGui::EndTable();
        }

        if (changed) isDirty_ = true;
    }

    void AudioContainerEditorController::DrawTracks()
    {
        if (ImGui::Button("+ Add New Track"))
        {
            audioContainer_->Tracks.emplace_back();
            isDirty_ = true;
        }

        ImGui::Spacing();

        const std::filesystem::path root = DetermineAudioRoot();
        std::vector<std::filesystem::path> audioFiles;
        CollectFiles(audioFiles, {".mp3", ".wav", ".ogg"});

        int indexToRemove = -1;

        
        if (ImGui::BeginTable("TracksTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable))
        {
            ImGui::TableSetupColumn("Clip", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Weight", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupColumn("Vol Mult", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupColumn("Pitch Mult", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableHeadersRow();

            for (int i = 0; i < audioContainer_->Tracks.size(); ++i)
            {
                auto& track = audioContainer_->Tracks[i];
                ImGui::PushID(i);
                ImGui::TableNextRow();

                
                ImGui::TableSetColumnIndex(0);
                std::string currentPath = track.Clip ? track.Clip->GetPath().ToStdString() : "";
                std::string preview = track.Clip ? MakeDisplayName(track.Clip->GetPath().ToStdString(), root) : "Select Audio Clip...";
                
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::BeginCombo("##Clip", preview.c_str()))
                {
                    if (ImGui::Selectable("<None>", !track.Clip))
                    {
                        track.Clip.reset();
                        isDirty_ = true;
                    }

                    for (const auto& file : audioFiles)
                    {
                        std::string display = MakeDisplayName(file, root);
                        bool selected = (currentPath == file.generic_string());
                        if (ImGui::Selectable(display.c_str(), selected))
                        {
                            track.Clip = resources::ResourceManager::Get().Get<resources::AudioClip>(file.generic_string());
                            isDirty_ = true;
                        }
                        if (selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                    {
                        const char* pathStr = (const char*)payload->Data;
                        std::filesystem::path p(pathStr);
                        std::string ext = p.extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
                        
                        if (ext == ".mp3" || ext == ".wav" || ext == ".ogg")
                        {
                            track.Clip = resources::ResourceManager::Get().Get<resources::AudioClip>(pathStr);
                            isDirty_ = true;
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::DragFloat("##Weight", &track.Weight, 0.1f, 0.0f, 100.0f, "%.1f")) isDirty_ = true;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Probability Weight");

                
                ImGui::TableSetColumnIndex(2);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::DragFloat("##Vol", &track.VolumeMultiplier, 0.01f, 0.0f, 2.0f, "%.2f")) isDirty_ = true;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Volume Multiplier");

                
                ImGui::TableSetColumnIndex(3);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::DragFloat("##Pitch", &track.PitchMultiplier, 0.01f, 0.1f, 3.0f, "%.2f")) isDirty_ = true;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pitch Multiplier");

                
                ImGui::TableSetColumnIndex(4);
                
                bool isPlayingThis = (previewState_->initialized && previewState_->trackIndex == i);
                if (isPlayingThis)
                {
                    if (ma_sound_at_end(&previewState_->sound))
                    {
                        
                        ma_sound_stop(&previewState_->sound);
                        ma_sound_uninit(&previewState_->sound);
                        previewState_->initialized = false;
                        previewState_->trackIndex = -1;
                        isPlayingThis = false;
                    }
                }

                if (isPlayingThis)
                {
                    if (ImGui::Button("Stop"))
                    {
                        ma_sound_stop(&previewState_->sound);
                        ma_sound_uninit(&previewState_->sound);
                        previewState_->initialized = false;
                        previewState_->trackIndex = -1;
                    }
                }
                else
                {
                    if (ImGui::Button("Play"))
                    {
                        if (track.Clip)
                        {
                            
                            if (previewState_->initialized)
                            {
                                ma_sound_stop(&previewState_->sound);
                                ma_sound_uninit(&previewState_->sound);
                                previewState_->initialized = false;
                            }

                            auto* engine = Systems::AudioSystem::Get().GetEngine();
                            if (engine)
                            {
                                ma_result result = ma_sound_init_from_file(
                                    engine,
                                    track.Clip->GetPath().c_str(),
                                    MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION,
                                    NULL,
                                    NULL,
                                    &previewState_->sound
                                );

                                if (result == MA_SUCCESS)
                                {
                                    ma_sound_set_volume(&previewState_->sound, track.VolumeMultiplier); 
                                    ma_sound_set_pitch(&previewState_->sound, track.PitchMultiplier);
                                    ma_sound_start(&previewState_->sound);
                                    previewState_->initialized = true;
                                    previewState_->trackIndex = i;
                                }
                            }
                        }
                    }
                }

                
                ImGui::TableSetColumnIndex(5);
                if (ImGui::Button("X"))
                {
                    indexToRemove = i;
                }

                ImGui::PopID();
            }

            ImGui::EndTable();
        }

        if (indexToRemove >= 0)
        {
            audioContainer_->Tracks.erase(audioContainer_->Tracks.begin() + indexToRemove);
            isDirty_ = true;
        }
    }
}

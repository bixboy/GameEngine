#include "Gui/Controllers/Windows/AudioContainerEditorWindow.h"
#include "Ressources/Core/ResourceManager.h"
#include "Systems/Audio/AudioSystem.h"
#include <imgui.h>
#include <filesystem>
#include <algorithm>
#include <miniaudio.h>


namespace BixEngine::Gui
{
    struct AudioContainerEditorWindow::PreviewState
    {
        ma_sound sound;
        bool initialized = false;
        int trackIndex = -1;
    };

    namespace
    {
        std::filesystem::path DetermineAudioRoot()
        {
            std::error_code ec;
            auto base = std::filesystem::current_path(ec);
            if (ec)
                return {};
    
            if (std::filesystem::exists(base / "Content"))
                return base / "Content";
            
            if (std::filesystem::exists(base / "Resources"))
                return base / "Resources";
            
            return base;
        }

        void CollectFiles(const std::filesystem::path& root, std::vector<std::filesystem::path>& outFiles, const std::vector<std::string>& extensions)
        {
            if (root.empty())
                return;
            
            outFiles.clear();

            std::error_code ec;
            for (auto it = std::filesystem::recursive_directory_iterator(root, ec); it != std::filesystem::recursive_directory_iterator(); it.increment(ec))
            {
                if (ec || !it->is_regular_file())
                    continue;

                std::string ext = it->path().extension().string();
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

            std::sort(outFiles.begin(), outFiles.end());
        }

        std::string MakeDisplayName(const std::filesystem::path& path, const std::filesystem::path& root)
        {
            if (root.empty())
                return path.filename().string();
            
            std::error_code ec;
            auto relative = std::filesystem::relative(path, root, ec);
            
            return (!ec && !relative.empty()) ? relative.string() : path.filename().string();
        }
    }

    // --- Factory ---
    std::shared_ptr<BaseAssetEditorWindow::SharedState> AudioContainerEditorWindow::CreateSharedState(
        std::filesystem::path assetPath, const String& stableId, std::function<void()> onCloseRequest)
    {
        auto state = std::make_shared<AudioEditorSharedState>();
        state->assetPath = assetPath;
        state->assetDisplayName = assetPath.filename().string();
        state->assetTypeLabel = "Audio Container";
        state->onCloseRequest = std::move(onCloseRequest);
        state->audioRoot = DetermineAudioRoot();
        
        state->container = Resources::ResourceManager::Get().Get<Resources::AudioContainer>(assetPath.string());
        
        if (!state->container)
        {
            state->container = std::make_shared<Resources::AudioContainer>();
            if (std::filesystem::exists(assetPath))
            {
                state->container->LoadFromFile(assetPath.string());
            }
        }

        CollectFiles(state->audioRoot, state->cachedAudioFiles, {".mp3", ".wav", ".ogg"});
        return state;
    }

    AudioContainerEditorWindow::AudioContainerEditorWindow(std::shared_ptr<SharedState> sharedState)
        : BaseAssetEditorWindow(std::move(sharedState), PanelConfig{ .titlePrefix = "Audio Editor", .dockRegion = DockSpaceRegion::Center })
    {
        previewState_ = std::make_unique<PreviewState>();
    }

    AudioContainerEditorWindow::~AudioContainerEditorWindow()
    {
        if (previewState_ && previewState_->initialized)
        {
            ma_sound_uninit(&previewState_->sound);
        }
    }

    void AudioContainerEditorWindow::OnSaveRequested()
    {
        auto state = std::static_pointer_cast<AudioEditorSharedState>(GetSharedState());
        if (state && state->container)
        {
            if (state->container->SaveToFile(state->assetPath.string()))
            {
                LOG_INFO("Audio Container saved successfully.");
                state->isDirty = false; 
            }
            else
            {
                LOG_ERROR("Failed to save Audio Container.");
            }
        }
    }

    void AudioContainerEditorWindow::DrawPanelContents(GuiPanel& panel)
    {
        (void)panel;
        auto state = std::static_pointer_cast<AudioEditorSharedState>(GetSharedState());
        
        if (!state || !state->container)
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: Invalid Audio Container State.");
            return;
        }

        DrawStandardToolbar();
        
        if (ImGui::BeginChild("AudioContent", ImVec2(0, 0), false))
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
        }
        ImGui::EndChild();
    }

    void AudioContainerEditorWindow::DrawProperties()
    {
        auto state = std::static_pointer_cast<AudioEditorSharedState>(GetSharedState());
        auto& container = state->container;
        bool changed = false;

        if (ImGui::BeginTable("PropertiesTable", 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("Value");
            
            // Mode
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Playback Mode");
            ImGui::TableSetColumnIndex(1);
            
            // Cast safe pour l'enum
            int mode = static_cast<int>(container->Mode);
            if (ImGui::Combo("##Mode", &mode, "Random\0Sequence\0"))
            {
                container->Mode = static_cast<Resources::AudioContainerMode>(mode);
                changed = true;
            }

            // Looping
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Looping");
            ImGui::TableSetColumnIndex(1);
            if (ImGui::Checkbox("##Loop", &container->Loop))
                changed = true;

            // Volume Variance
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Volume Variance");
            ImGui::TableSetColumnIndex(1);
            if (ImGui::DragFloat("##VolVar", &container->VolumeVariance, 0.01f, 0.0f, 1.0f, "%.2f"))
                changed = true;
            
            // Pitch Variance
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Pitch Variance");
            ImGui::TableSetColumnIndex(1);
            if (ImGui::DragFloat("##PitchVar", &container->PitchVariance, 0.01f, 0.0f, 1.0f, "%.2f"))
                changed = true;

            ImGui::EndTable();
        }

        if (changed)
            state->isDirty = true;
    }

    void AudioContainerEditorWindow::RefreshAudioFilesCache()
    {
        auto state = std::static_pointer_cast<AudioEditorSharedState>(GetSharedState());
        
        if(state)
            CollectFiles(state->audioRoot, state->cachedAudioFiles, {".mp3", ".wav", ".ogg"});
    }

    void AudioContainerEditorWindow::DrawTracks()
    {
        auto state = std::static_pointer_cast<AudioEditorSharedState>(GetSharedState());
        auto& container = state->container;
        bool changed = false;

        // Boutons d'action
        if (ImGui::Button("+ Add New Track"))
        {
            container->Tracks.emplace_back();
            container->Tracks.back().Weight = 1.0f;
            container->Tracks.back().VolumeMultiplier = 1.0f;
            container->Tracks.back().PitchMultiplier = 1.0f;
            changed = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Refresh Files"))
        {
            RefreshAudioFilesCache();
        }

        ImGui::Spacing();

        int indexToRemove = -1;

        if (ImGui::BeginTable("TracksTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("Clip", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Weight", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Vol", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Pitch", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Play", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableHeadersRow();

            for (int i = 0; i < (int)container->Tracks.size(); ++i)
            {
                auto& track = container->Tracks[i];
                ImGui::PushID(i);
                ImGui::TableNextRow();

                // 1. Clip Selector (Combo + DragDrop)
                ImGui::TableSetColumnIndex(0);
                std::string currentPath = track.Clip ? track.Clip->GetPath().Std() : "";
                std::string previewName = track.Clip ? MakeDisplayName(track.Clip->GetPath().Std(), state->audioRoot) : "<Empty>";
                
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::BeginCombo("##Clip", previewName.c_str()))
                {
                    if (ImGui::Selectable("<None>", !track.Clip))
                    {
                        track.Clip.reset();
                        changed = true;
                    }
                    
                    for (const auto& file : state->cachedAudioFiles)
                    {
                        bool selected = (currentPath == file.string());
                        if (ImGui::Selectable(MakeDisplayName(file, state->audioRoot).c_str(), selected))
                        {
                            track.Clip = Resources::ResourceManager::Get().Get<Resources::AudioClip>(file.string());
                            changed = true;
                        }
                        
                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    
                    ImGui::EndCombo();
                }

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                    {
                        const char* pathStr = static_cast<const char*>(payload->Data);
                        track.Clip = Resources::ResourceManager::Get().Get<Resources::AudioClip>(pathStr);
                        changed = true;
                    }
                    ImGui::EndDragDropTarget();
                }

                // 2. Weight
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::DragFloat("##Weight", &track.Weight, 0.1f, 0.0f, 100.0f, "%.1f"))
                    changed = true;
                
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Probability Weight (Random Mode)");

                // 3. Volume
                ImGui::TableSetColumnIndex(2);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::DragFloat("##Vol", &track.VolumeMultiplier, 0.01f, 0.0f, 2.0f, "%.2f"))
                    changed = true;

                // 4. Pitch
                ImGui::TableSetColumnIndex(3);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::DragFloat("##Pitch", &track.PitchMultiplier, 0.01f, 0.1f, 3.0f, "%.2f"))
                    changed = true;

                // 5. Preview Logic
                ImGui::TableSetColumnIndex(4);
                bool isPlayingThis = (previewState_->initialized && previewState_->trackIndex == i);

                // Check si le son est fini
                if (isPlayingThis && ma_sound_at_end(&previewState_->sound))
                {
                    ma_sound_stop(&previewState_->sound);
                    ma_sound_uninit(&previewState_->sound);
                    previewState_->initialized = false;
                    previewState_->trackIndex = -1;
                    isPlayingThis = false;
                }

                if (isPlayingThis) {
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
                    ImGui::BeginDisabled(track.Clip == nullptr);
                    if (ImGui::Button("Play"))
                    {
                        if (previewState_->initialized)
                        {
                            ma_sound_stop(&previewState_->sound);
                            ma_sound_uninit(&previewState_->sound);
                        }
                        
                        auto* engine = Systems::AudioSystem::Get().GetEngine();
                        if (engine && track.Clip)
                        {
                            ma_result res = ma_sound_init_from_file(engine, track.Clip->GetPath().c_str(), 
                                MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr, nullptr, &previewState_->sound);
                            
                            if (res == MA_SUCCESS)
                            {
                                ma_sound_set_volume(&previewState_->sound, track.VolumeMultiplier);
                                ma_sound_set_pitch(&previewState_->sound, track.PitchMultiplier);
                                ma_sound_start(&previewState_->sound);
                                previewState_->initialized = true;
                                previewState_->trackIndex = i;
                            }
                            else
                            {
                                LOG_ERROR("Failed to preview audio: " + track.Clip->GetPath().Std());
                            }
                        }
                    }
                    
                    ImGui::EndDisabled();
                }

                // 6. Delete
                ImGui::TableSetColumnIndex(5);
                if (ImGui::Button("X"))
                    indexToRemove = i;

                ImGui::PopID();
            }
            
            ImGui::EndTable();
        }

        if (indexToRemove >= 0)
        {
            container->Tracks.erase(container->Tracks.begin() + indexToRemove);
            changed = true;
        }

        if (changed)
            state->isDirty = true;
    }
}
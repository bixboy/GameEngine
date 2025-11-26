#include "Gui/Internal/NavBar/GuiNavigationBar.h"
#include "Systems/AudioSystem.h"

#include <string_view>

#include "Gui/Internal/GuiModule.h"
#include "Gui/Internal/NavBar/GuiAssetEditorManager.h"
#include "Gui/Internal/GuiSystem.h"
#include "Gui/Internal/GuiLayoutManager.h"
#include "Gui/Panels/GuiPanel.h"
#include "imgui.h"
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>

namespace BixEngine::Core
{
    namespace
    {
        constexpr float kNavigationBarHeight = 38.0f;
        constexpr std::string_view kSceneNavigationId{"scene"};

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

        void CollectAudioFiles(std::vector<std::filesystem::path>& outFiles)
        {
            const std::filesystem::path root = DetermineAudioRoot();
            if (root.empty()) return;

            std::error_code ec;
            for (std::filesystem::recursive_directory_iterator it(root, ec), end; it != end; it.increment(ec))
            {
                if (ec) break;
                if (!it->is_regular_file()) continue;

                std::string extension = it->path().extension().generic_string();
                std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c){ return std::tolower(c); });
                
                if (extension == ".mp3" || extension == ".wav" || extension == ".ogg")
                    outFiles.push_back(it->path());
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

    GuiNavigationBar::GuiNavigationBar(GuiSystem& guiSystem, GuiLayoutManager& layoutManager, GuiModule& owner)
        : guiSystem_(&guiSystem), layoutManager_(&layoutManager), owner_(&owner)
    {
    }

    void GuiNavigationBar::Render()
    {
        if (!guiSystem_ || !guiSystem_->IsInitialized())
            return;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        if (!viewport)
            return;

        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y));
        ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, kNavigationBarHeight));
        ImGui::SetNextWindowViewport(viewport->ID);

        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoCollapse;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{0.08f, 0.08f, 0.09f, 0.96f});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 6.0f));

        if (ImGui::Begin("EditorNavigationBar", nullptr, flags))
        {
            if (layoutManager_)
            {
                if (auto* manager = owner_->GetAssetEditorManager())
                    manager->OnLayoutChanged(layoutManager_->GetCurrentLayout());
            }

            constexpr float buttonHeight = kNavigationBarHeight - 16.0f;

            DrawSceneButton(buttonHeight);
            DrawPlayControls(buttonHeight);
            DrawAssetEditorTabs(buttonHeight);
            DrawAudioPlayer(buttonHeight);
        }

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

    bool GuiNavigationBar::DrawNavigationButton(const std::string& label, bool isActive, float buttonHeight) const
    {
        if (isActive)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.30f, 0.30f, 0.34f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.34f, 0.34f, 0.38f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.28f, 0.28f, 0.32f, 1.0f});
        }

        const ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        constexpr float paddingX = 32.0f;
        const ImVec2 size{textSize.x + paddingX, buttonHeight};
        const bool clicked = ImGui::Button(label.c_str(), size);

        if (isActive)
            ImGui::PopStyleColor(3);

        return clicked;
    }

    bool GuiNavigationBar::DrawCloseButton(std::string_view label, float buttonHeight) const
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 4.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.42f, 0.12f, 0.12f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.58f, 0.16f, 0.16f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.36f, 0.10f, 0.10f, 1.0f});

        const float closeButtonSize = buttonHeight - 12.0f;
        const ImVec2 closeSize{std::max(12.0f, closeButtonSize), buttonHeight};
        const bool closeRequested = ImGui::Button("x", closeSize);

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Close %.*s", static_cast<int>(label.size()), label.data());

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        return closeRequested;
    }

    void GuiNavigationBar::DrawSceneButton(float buttonHeight)
    {
        static const std::string kSceneLabel{"Scene"};
        auto* manager = owner_->GetAssetEditorManager();
        const bool sceneActive = !manager || manager->GetActiveNavigationId() == kSceneNavigationId || !manager->
            HasEditors();

        if (!DrawNavigationButton(kSceneLabel, sceneActive, buttonHeight))
            return;

        if (manager)
            manager->ActivateScene(true);
        else
            owner_->FocusSceneViewport();
    }

    void GuiNavigationBar::DrawAssetEditorTabs(float buttonHeight)
    {
        auto* manager = owner_->GetAssetEditorManager();
        if (!manager || manager->GetEditorOrder().empty())
            return;

        std::vector<std::string> closeRequests;
        std::vector<std::string> staleEntries;
        closeRequests.reserve(manager->GetEditorOrder().size());
        staleEntries.reserve(manager->GetEditorOrder().size());

        for (const std::string& navId : manager->GetEditorOrder())
        {
            auto* entry = manager->FindEditor(navId);
            if (!entry)
            {
                staleEntries.push_back(navId);
                continue;
            }

            if (entry->sharedState)
            {
                const std::string displayName = entry->sharedState->assetDisplayName.Std();
                if (entry->buttonLabel != displayName)
                    entry->buttonLabel = displayName;
            }

            ImGui::SameLine();
            const bool isActive = manager->GetActiveNavigationId() == entry->navigationId;

            ImGui::PushID(entry->navigationId.c_str());

            const std::string& label = entry->buttonLabel.empty() ? entry->navigationId : entry->buttonLabel;
            if (DrawNavigationButton(label, isActive, buttonHeight))
                manager->ActivateEditor(entry->navigationId, true);

            ImGui::SameLine(0.0f, 6.0f);
            if (DrawCloseButton(label, buttonHeight))
                closeRequests.push_back(entry->navigationId);

            ImGui::PopID();
        }

        for (const std::string& stale : staleEntries)
            manager->CloseAssetEditor(stale);

        for (const std::string& navigationId : closeRequests)
            manager->CloseAssetEditor(navigationId);
    }

    void GuiNavigationBar::DrawPlayControls(float buttonHeight)
    {
        ImGui::SameLine();
        
        // Center the controls
        const float width = ImGui::GetWindowWidth();
        const float controlsWidth = 150.0f; // Approximate
        ImGui::SetCursorPosX((width - controlsWidth) * 0.5f);

        auto state = owner_->GetEngineState();
        bool isPlay = state == GuiModule::EngineState::Play;
        bool isPause = state == GuiModule::EngineState::Pause;
        bool isEdit = state == GuiModule::EngineState::Edit;

        // Play Button
        if (isEdit)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
            if (ImGui::Button("Play", ImVec2(50, buttonHeight)))
                owner_->OnPlay();
            ImGui::PopStyleColor();
        }
        else
        {
            // Stop Button
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("Stop", ImVec2(50, buttonHeight)))
                owner_->OnStop();
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();

        // Pause Button
        if (!isEdit)
        {
            if (ImGui::Button(isPause ? "Resume" : "Pause", ImVec2(60, buttonHeight)))
                owner_->OnPause();
        }
    }

    void GuiNavigationBar::DrawAudioPlayer(float buttonHeight)
    {
        // Estimated width: Combo(150) + Play(30) + Stop(30) + Progress(100) + Slider(80) + Spacing
        constexpr float estimatedWidth = 450.0f; 
        constexpr float padding = 20.0f;

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - estimatedWidth - padding);

        // Collect Audio Files (Note: In a real engine, this should be cached)
        std::vector<std::filesystem::path> audioFiles;
        CollectAudioFiles(audioFiles);
        const std::filesystem::path root = DetermineAudioRoot();

        std::vector<std::string> audioPaths;
        std::vector<std::string> audioLabels;
        audioPaths.reserve(audioFiles.size());
        audioLabels.reserve(audioFiles.size());

        int currentAudioIndex = -1;
        
        for (size_t index = 0; index < audioFiles.size(); ++index)
        {
            const std::string pathString = audioFiles[index].generic_string();
            audioPaths.push_back(pathString);
            audioLabels.push_back(MakeDisplayName(audioFiles[index], root));

            // Check if this is the current song
            // We compare full paths or just check if current song name matches
            if (m_CurrentSongName == pathString)
            {
                currentAudioIndex = static_cast<int>(index);
            }
        }

        // Combo Box
        ImGui::SetNextItemWidth(150.0f);
        const char* previewValue = currentAudioIndex >= 0 ? audioLabels[currentAudioIndex].c_str() : (m_CurrentSongName == "No Audio" ? "Select Audio..." : m_CurrentSongName.c_str());
        if (ImGui::BeginCombo("##AudioSelect", previewValue))
        {
            for (int i = 0; i < static_cast<int>(audioPaths.size()); ++i)
            {
                const bool isSelected = (currentAudioIndex == i);
                if (ImGui::Selectable(audioLabels[i].c_str(), isSelected))
                {
                    m_CurrentSongName = audioPaths[i];
                    Systems::AudioSystem::Get().PlayMusic(m_CurrentSongName);
                    m_IsPlaying = true;
                }

                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Select Music");

        ImGui::SameLine();

        // Play/Pause Button
        if (!m_IsPlaying)
        {
            if (ImGui::ArrowButton("##PlayMusic", ImGuiDir_Right))
            {
                Systems::AudioSystem::Get().PlayMusic(m_CurrentSongName);
                m_IsPlaying = true;
            }
        }
        else
        {
            if (ImGui::Button("||", ImVec2(20, 0))) 
            {
                Systems::AudioSystem::Get().Pause();
                m_IsPlaying = false;
            }
        }

        ImGui::SameLine();

        // Stop Button
        if (ImGui::Button("[]", ImVec2(20, 0)))
        {
            Systems::AudioSystem::Get().Stop();
            m_IsPlaying = false;
        }

        ImGui::SameLine();

        // Progress Bar
        float duration = Systems::AudioSystem::Get().GetMusicDuration();
        float cursor = Systems::AudioSystem::Get().GetMusicCursor();
        float progress = (duration > 0.0f) ? (cursor / duration) : 0.0f;
        
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
        ImGui::ProgressBar(progress, ImVec2(100, 0), "");
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%.1f / %.1f s", cursor, duration);

        ImGui::SameLine();

        // Volume Slider
        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::SliderFloat("##Vol", &m_MasterVolume, 0.0f, 1.0f, ""))
        {
            Systems::AudioSystem::Get().SetGlobalVolume(m_MasterVolume);
        }
    }
}

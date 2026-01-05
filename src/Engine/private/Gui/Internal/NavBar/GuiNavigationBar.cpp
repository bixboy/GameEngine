#include "Gui/Internal/NavBar/GuiNavigationBar.h"
#include "Systems/Audio/AudioSystem.h"
#include <string_view>
#include "Gui/Internal/GuiModule.h"
#include "Gui/Internal/NavBar/GuiAssetEditorManager.h"
#include "Gui/Internal/GuiSystem.h"
#include "Gui/Internal/GuiLayoutManager.h"
#include "Utils/FileIO/FilesUtils.h"
#include "imgui.h"
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include "Ressources/RessourcesClass/AudioContainer.h"
#include "Ressources/RessourcesClass/AudioClip.h"
#include "Ressources/Core/ResourceManager.h"


namespace BixEngine::Gui
{
    namespace
    {
        constexpr float kNavigationBarHeight = 38.0f;
        constexpr std::string_view kSceneNavigationId{"scene"};

        std::filesystem::path DetermineAudioRoot()
        {
            std::error_code ec;
            const std::filesystem::path base = std::filesystem::current_path(ec);
            if (ec)
                return {};

            const std::filesystem::path content = base / "Content";
            if (std::filesystem::exists(content))
                return content;

            const std::filesystem::path resources = base / "Resources";
            if (std::filesystem::exists(resources))
                return resources;

            return {};
        }

        void CollectAudioFiles(std::vector<std::filesystem::path>& outFiles)
        {
            const std::filesystem::path root = DetermineAudioRoot();
            if (root.empty())
                return;

            const std::vector<std::string> audioExtensions = { ".mp3", ".wav", ".ogg", ".bixaudio" };
            outFiles = Utils::FileUtils::ScanDirectory(root, audioExtensions, true);

            // ScanDirectory doesn't guarantee order, and we might want to sort by generic string as before
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
            ImGui::SameLine();

            
            
            
            
            const float windowWidth = ImGui::GetWindowWidth();
            const float centerX = windowWidth * 0.5f;
            
            
            
            
            
            bool showPlayControls = false;
            if (auto* manager = owner_->GetAssetEditorManager())
            {
                showPlayControls = (manager->GetActiveLayout() == DefaultLayouts::Scene);
            }

            float tabsEndX = windowWidth - 460.0f; 
            if (showPlayControls)
            {
                
                
                tabsEndX = std::min(tabsEndX, centerX - 80.0f);
            }

            float currentX = ImGui::GetCursorPosX();
            float tabsWidth = std::max(10.0f, tabsEndX - currentX);

            DrawAssetEditorTabs(buttonHeight, tabsWidth);

            
            if (showPlayControls)
            {
                DrawPlayControls(buttonHeight);
            }

            
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

    void GuiNavigationBar::DrawAssetEditorTabs(float buttonHeight, float availableWidth)
    {
        auto* manager = owner_->GetAssetEditorManager();
        if (!manager || manager->GetEditorOrder().empty())
            return;

        
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0,0,0,0));
        if (ImGui::BeginChild("AssetTabsRegion", ImVec2(availableWidth, buttonHeight + 10.0f), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoBackground))
        {
            std::vector<std::string> closeRequests;
            std::vector<std::string> staleEntries;
            closeRequests.reserve(manager->GetEditorOrder().size());
            staleEntries.reserve(manager->GetEditorOrder().size());

            bool first = true;
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

                if (!first)
                    ImGui::SameLine();
                
                first = false;

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
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    void GuiNavigationBar::DrawPlayControls(float buttonHeight)
    {
        
        const float width = ImGui::GetWindowWidth();
        const float controlsWidth = 150.0f; 
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        ImGui::SameLine();
        ImGui::SetCursorPosX((width - controlsWidth) * 0.5f);

        auto state = owner_->GetEngineState();
        
        bool isPause = state == GuiModule::EngineState::Pause;
        bool isEdit = state == GuiModule::EngineState::Edit;

        
        if (isEdit)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
            if (ImGui::Button("Play", ImVec2(50, buttonHeight)))
                owner_->OnPlay();
            
            ImGui::PopStyleColor();
        }
        else
        {
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("Stop", ImVec2(50, buttonHeight)))
                owner_->OnStop();
            
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();

        
        if (!isEdit)
        {
            if (ImGui::Button(isPause ? "Resume" : "Pause", ImVec2(60, buttonHeight)))
                owner_->OnPause();
        }
    }

    void GuiNavigationBar::DrawAudioPlayer(float buttonHeight)
    {
        // Update audio list periodically
        m_AudioScanTimer += ImGui::GetIO().DeltaTime;
        if (m_AudioListDirty || (m_AudioScanTimer > 2.0f && !ImGui::IsPopupOpen("##AudioSelect")))
        {
            UpdateAudioFileList();
        }

        constexpr float kComboWidth = 120.0f; 
        constexpr float kButtonSize = 18.0f;  
        constexpr float kProgressWidth = 80.0f; 
        constexpr float kVolumeWidth = 60.0f;   
        constexpr float kItemSpacing = 4.0f;    
        constexpr float kPaddingX = 8.0f;       
        constexpr float kPaddingY = 6.0f;
        
        const float totalContentWidth = kComboWidth + kItemSpacing + kButtonSize + kItemSpacing + kButtonSize +
            kItemSpacing + kProgressWidth + kItemSpacing + kVolumeWidth;
        
        const float totalRectWidth = totalContentWidth + (kPaddingX * 2.0f) + 20.0f; 
        const float rectHeight = 32.0f; 

        ImGui::SameLine();
        
        const float startX = ImGui::GetWindowWidth() - totalRectWidth - 10.0f;
        
        ImGui::SetCursorPosX(startX);
        
        const float windowHeight = ImGui::GetWindowHeight();
        const float containerY = (windowHeight - rectHeight) * 0.5f;
        
        ImGui::SetCursorPosY(containerY);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 pMin = ImGui::GetCursorScreenPos();
        ImVec2 pMax = ImVec2(pMin.x + totalRectWidth, pMin.y + rectHeight);
        
        drawList->AddRectFilled(pMin, pMax, IM_COL32(30, 30, 35, 255), 6.0f);
        drawList->AddRect(pMin, pMax, IM_COL32(60, 60, 65, 255), 6.0f);

        const float contentHeight = ImGui::GetFrameHeight(); 
        const float contentY = containerY + (rectHeight - contentHeight) * 0.5f;
        
        ImGui::SetCursorPosX(startX + kPaddingX);
        ImGui::SetCursorPosY(contentY);

        int currentAudioIndex = -1;
        for (size_t index = 0; index < m_CachedAudioPaths.size(); ++index)
        {
            if (m_CurrentSongName == m_CachedAudioPaths[index])
            {
                currentAudioIndex = static_cast<int>(index);
                break;
            }
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.17f, 1.0f));
        
        ImGui::SetNextItemWidth(kComboWidth);
        const char* previewValue = currentAudioIndex >= 0 ? m_CachedAudioLabels[currentAudioIndex].c_str() : (m_CurrentSongName == "No Audio" ? "Select Audio..." : "Unknown");
        
        std::string previewStr = previewValue;
        if (previewStr.size() > 20) previewStr = previewStr.substr(0, 17) + "...";

        ImGui::SetCursorPosY(contentY); 
        if (ImGui::BeginCombo("##AudioSelect", previewStr.c_str(), ImGuiComboFlags_HeightLarge))
        {
            for (int i = 0; i < static_cast<int>(m_CachedAudioPaths.size()); ++i)
            {
                const bool isSelected = (currentAudioIndex == i);
                if (ImGui::Selectable(m_CachedAudioLabels[i].c_str(), isSelected))
                {
                    m_CurrentSongName = m_CachedAudioPaths[i];
                    
                    std::string playPath = m_CurrentSongName;
                    if (std::filesystem::path(playPath).extension() == ".bixaudio")
                    {
                        if (auto container = Resources::ResourceManager::Get().Get<Resources::AudioContainer>(playPath.c_str()))
                        {
                            auto resolved = container->ResolveSound();
                            if (resolved.Clip)
                            {
                                playPath = resolved.Clip->GetPath().Std();
                            }
                        }
                    }

                    Systems::AudioSystem::Get().PlayMusic(playPath);
                    m_IsPlaying = true;
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", m_CurrentSongName.c_str());

        ImGui::SameLine(0, kItemSpacing);
        ImGui::SetCursorPosY(contentY); 
        
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); 
        if (!m_IsPlaying)
        {
            if (ImGui::ArrowButton("##PlayMusic", ImGuiDir_Right))
            {
                std::string playPath = m_CurrentSongName;
                if (std::filesystem::path(playPath).extension() == ".bixaudio")
                {
                    if (auto container = Resources::ResourceManager::Get().Get<Resources::AudioContainer>(playPath.c_str()))
                    {
                        auto resolved = container->ResolveSound();
                        if (resolved.Clip)
                        {
                            playPath = resolved.Clip->GetPath().Std();
                        }
                    }
                }
                Systems::AudioSystem::Get().PlayMusic(playPath);
                m_IsPlaying = true;
            }
        }
        else
        {
            if (ImGui::Button("||", ImVec2(kButtonSize, 0))) 
            {
                Systems::AudioSystem::Get().Pause();
                m_IsPlaying = false;
            }
        }
        ImGui::PopStyleColor();

        ImGui::SameLine(0, kItemSpacing);
        ImGui::SetCursorPosY(contentY); 
        
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        
        if (ImGui::Button("[]", ImVec2(kButtonSize, 0)))
        {
            Systems::AudioSystem::Get().Stop();
            m_IsPlaying = false;
        }
        
        ImGui::PopStyleColor(2);

        ImGui::SameLine(0, kItemSpacing);
        ImGui::SetCursorPosY(contentY); 
        
        float duration = Systems::AudioSystem::Get().GetMusicDuration();
        float cursor = Systems::AudioSystem::Get().GetMusicCursor();
        float progress = (duration > 0.0f) ? (cursor / duration) : 0.0f;
        
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.7f, 0.3f, 1.0f)); 
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        
        ImGui::ProgressBar(progress, ImVec2(kProgressWidth, 0), "");
        ImGui::PopStyleColor(2);
        
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%.1f / %.1f s", cursor, duration);

        ImGui::SameLine(0, kItemSpacing);
        ImGui::SetCursorPosY(contentY); 
        
        ImGui::SetNextItemWidth(kVolumeWidth);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        
        if (ImGui::SliderFloat("##Vol", &m_MasterVolume, 0.0f, 1.0f, ""))
        {
            Systems::AudioSystem::Get().SetGlobalVolume(m_MasterVolume);
        }
        
        ImGui::PopStyleColor(2);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Volume: %.0f%%", m_MasterVolume * 100.0f);

        ImGui::PopStyleVar(); 
    }

    void GuiNavigationBar::UpdateAudioFileList()
    {
        m_CachedAudioFiles.clear();
        m_CachedAudioLabels.clear();
        m_CachedAudioPaths.clear();

        CollectAudioFiles(m_CachedAudioFiles);
        const std::filesystem::path root = DetermineAudioRoot();

        m_CachedAudioFiles.reserve(m_CachedAudioFiles.size());
        m_CachedAudioLabels.reserve(m_CachedAudioFiles.size());
        m_CachedAudioPaths.reserve(m_CachedAudioFiles.size());

        for (const auto& file : m_CachedAudioFiles)
        {
            m_CachedAudioPaths.push_back(file.generic_string());
            m_CachedAudioLabels.push_back(MakeDisplayName(file, root));
        }

        m_AudioListDirty = false;
        m_AudioScanTimer = 0.0f;
    }
}

#include "Engine/Gui/Internal/GuiSystem.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include <SDL3/SDL.h>

#include "Core/Logger.h"
#include "Engine/Gui/Internal/GuiPanel.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "imgui_internal.h"
#include "Engine/Gui/Internal/GuiDocking.h"

namespace BixEngine::Gui
{
    GuiSystem::~GuiSystem()
    {
        Shutdown();
    }

    // ────────────────────────────────────────────────
    // ⚙️ Initialisation / Shutdown
    // ────────────────────────────────────────────────
    
    bool GuiSystem::Initialize(SDL_Window* window, SDL_Renderer* renderer)
    {
        if (initialized_)
            return true;

        if (!window || !renderer)
        {
            LOG_ERROR("GuiSystem initialization failed: invalid SDL window or renderer.");
            return false;
        }

        window_ = window;
        renderer_ = renderer;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;

        dockingEnabled_ = (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) != 0;
        useSavedDockLayout_ = dockingEnabled_ && HasSavedDockLayout_();

        ImGui::StyleColorsDark();

        if (!ImGui_ImplSDL3_InitForSDLRenderer(window_, renderer_) ||
            !ImGui_ImplSDLRenderer3_Init(renderer_))
        {
            LOG_ERROR("Failed to initialize ImGui SDL3 backend: " + std::string(SDL_GetError()));
            Shutdown();
            return false;
        }

        initialized_ = true;
        dockLayoutBuilt_ = useSavedDockLayout_;
        rebuildDockLayout_ = !useSavedDockLayout_;
        dockRegionIds_.fill(0);
        pendingDockUpdates_.clear();

        LOG_INFO("[GuiSystem] ✅ Initialized successfully.");
        return true;
    }

    void GuiSystem::Shutdown() noexcept
    {
        if (!initialized_)
            return;

        LOG_INFO("[GuiSystem] 🧹 Shutdown requested...");

        panels_.clear();
        pendingDockUpdates_.clear();

        if (ImGuiContext* context = ImGui::GetCurrentContext())
        {
            ImGui::SetCurrentContext(context);

            // 🔹 Sauvegarde les settings avant toute destruction
            ImGuiIO& io = ImGui::GetIO();
            if (io.IniFilename && *io.IniFilename)
            {
                LOG_INFO("[GuiSystem] 💾 Saving ImGui layout...");
                ImGui::SaveIniSettingsToDisk(io.IniFilename);
            }

            // 🔹 Détruit toutes les Platform Windows avant de tuer les backends
            LOG_INFO("[GuiSystem] 🪟 Destroying ImGui platform windows...");
            ImGui::DestroyPlatformWindows();
        }

        // 🔹 Ferme les backends ImGui avant le contexte
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();

        // 🔹 Supprime le contexte global ImGui
        if (ImGui::GetCurrentContext())
        {
            LOG_INFO("[GuiSystem] 🧠 Destroying ImGui context...");
            ImGui::DestroyContext();
        }

        // 🔹 Réinitialise l'état interne
        initialized_ = false;
        frameBegun_ = false;
        dockingEnabled_ = false;
        dockLayoutBuilt_ = false;
        rebuildDockLayout_ = true;
        dockspaceId_ = 0;
        dockRegionIds_.fill(0);
        window_ = nullptr;
        renderer_ = nullptr;
        useSavedDockLayout_ = false;

        LOG_INFO("[GuiSystem] ✅ Shutdown completed cleanly.");
    }


    void GuiSystem::SetDockspaceIdentifiers(std::string windowName, std::string dockspaceLabel)
    {
        dockspaceWindowName_ = windowName.empty() ? "EngineDockSpace" : std::move(windowName);
        dockspaceLabel_ = dockspaceLabel.empty() ? dockspaceWindowName_ + "::DockSpace" : std::move(dockspaceLabel);
        dockspaceId_ = 0;
    }

    void GuiSystem::RequestDefaultDockLayout()
    {
        useSavedDockLayout_ = false;
        dockLayoutBuilt_ = false;
        rebuildDockLayout_ = true;
        dockspaceId_ = 0;
        dockRegionIds_.fill(0);
        pendingDockUpdates_.clear();
    }

    std::string GuiSystem::SaveLayoutToMemory() const
    {
        if (!ImGui::GetCurrentContext())
            return {};

        size_t dataSize = 0;
        const char* iniData = ImGui::SaveIniSettingsToMemory(&dataSize);
        if (!iniData || dataSize == 0)
            return {};

        return std::string(iniData, dataSize);
    }

    void GuiSystem::LoadLayoutFromMemory(const std::string& data)
    {
        if (!ImGui::GetCurrentContext())
            return;

        if (data.empty())
        {
            RequestDefaultDockLayout();
            return;
        }

        ImGui::LoadIniSettingsFromMemory(data.c_str(), data.size());
        useSavedDockLayout_ = true;
        dockLayoutBuilt_ = true;
        rebuildDockLayout_ = false;
        pendingDockUpdates_.clear();

        for (GuiPanel* panel : panels_)
        {
            if (panel)
                panel->ResetDockId();
        }
    }


    // ────────────────────────────────────────────────
    // 🧠 Cycle de frame ImGui
    // ────────────────────────────────────────────────
    
    void GuiSystem::BeginFrame()
    {
        if (!initialized_ || !ImGui::GetCurrentContext())
            return;

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (dockingEnabled_)
        {
            BeginDockspaceLayout_();
            ApplyDockingPreferences_();
        }

        frameBegun_ = true;
    }

    void GuiSystem::EndFrame()
    {
        if (initialized_ && frameBegun_)
        {
            ImGui::EndFrame();
            frameBegun_ = false;
        }
    }

    void GuiSystem::Render()
    {
        if (!initialized_)
            return;

        if (frameBegun_)
            EndFrame();

        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer_);

        if (bShowDockDebugOverlay_)
            DumpGuiState();
    }

    // ────────────────────────────────────────────────
    // 🎮 Événements SDL
    // ────────────────────────────────────────────────
    
    void GuiSystem::ProcessEvent(const SDL_Event& event)
    {
        if (initialized_)
            ImGui_ImplSDL3_ProcessEvent(&event);
    }

    // ────────────────────────────────────────────────
    // 🪟 Gestion des panneaux
    // ────────────────────────────────────────────────
    
    void GuiSystem::RegisterPanel(GuiPanel& panel)
    {
        if (std::find(panels_.begin(), panels_.end(), &panel) == panels_.end())
        {
            panels_.push_back(&panel);
            panel.ResetDockId();

            if (dockingEnabled_ && !useSavedDockLayout_)
                QueuePanelForDockUpdate_(panel);
        }
    }

    void GuiSystem::UnregisterPanel(GuiPanel& panel)
    {
        panels_.erase(std::remove(panels_.begin(), panels_.end(), &panel), panels_.end());
        RemovePanelFromDockQueue_(panel);
    }

    void GuiSystem::EnqueueDockUpdate(GuiPanel& panel)
    {
        if (dockingEnabled_)
            QueuePanelForDockUpdate_(panel);
    }

    // ────────────────────────────────────────────────
    // 🧩 Docking & layout
    // ────────────────────────────────────────────────
    
    void GuiSystem::BeginDockspaceLayout_()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        if (!viewport)
            return;

        const ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar;
        const ImGuiWindowFlags winFlags =
            ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

        ImVec2 dockspacePos = viewport->WorkPos;
        ImVec2 dockspaceSize = viewport->WorkSize;
        if (dockspaceTopPadding_ > 0.0f)
        {
            dockspacePos.y += dockspaceTopPadding_;
            dockspaceSize.y = std::max(0.0f, dockspaceSize.y - dockspaceTopPadding_);
        }

        ImGui::SetNextWindowPos(dockspacePos);
        ImGui::SetNextWindowSize(dockspaceSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

        const char* dockspaceWindowName = dockspaceWindowName_.empty() ? "EngineDockSpace" : dockspaceWindowName_.c_str();
        ImGui::Begin(dockspaceWindowName, nullptr, winFlags);
        ImGui::PopStyleVar(3);

        const char* dockLabel = dockspaceLabel_.empty() ? "EngineDockSpace::DockSpace" : dockspaceLabel_.c_str();
        dockspaceId_ = ImGui::GetID(dockLabel);

        if (rebuildDockLayout_)
        {
            BuildDefaultDockLayout_(*viewport, dockspaceId_, dockFlags, dockspacePos, dockspaceSize);
            rebuildDockLayout_ = false;
        }

        ImGui::DockSpace(dockspaceId_, ImVec2(0.f, 0.f), dockFlags);
        ImGui::End();
    }

    void GuiSystem::BuildDefaultDockLayout_(ImGuiViewport& viewport,
                                            ImGuiID dockspaceId,
                                            ImGuiDockNodeFlags flags,
                                            const ImVec2& dockspacePos,
                                            const ImVec2& dockspaceSize)
    {
        static_cast<void>(viewport);
        if (dockspaceId == 0)
            return;

        dockRegionIds_.fill(0);

        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, flags | ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodePos(dockspaceId, dockspacePos);
        ImGui::DockBuilderSetNodeSize(dockspaceId, dockspaceSize);

        ImGuiID main = dockspaceId;
        ImGuiID left = ImGui::DockBuilderSplitNode(main, ImGuiDir_Left, 0.20f, nullptr, &main);
        ImGuiID right = ImGui::DockBuilderSplitNode(main, ImGuiDir_Right, 0.25f, nullptr, &main);
        ImGuiID bottom = ImGui::DockBuilderSplitNode(main, ImGuiDir_Down, 0.25f, nullptr, &main);
        ImGuiID top = ImGui::DockBuilderSplitNode(main, ImGuiDir_Up, 0.15f, nullptr, &main);

        dockRegionIds_ = {main, left, right, bottom, top};

        ImGui::DockBuilderFinish(dockspaceId);
        dockLayoutBuilt_ = true;

        QueueAllPanelsForDockUpdate_();
        LOG_INFO("[GuiSystem] 🧩 Default dock layout built.");
    }

    // ────────────────────────────────────────────────
    // 🧠 Debug / état interne
    // ────────────────────────────────────────────────
    
    void GuiSystem::DumpGuiState() const
    {
        LOG_INFO("───────────────────────────────");
        LOG_INFO("[GuiSystem] State dump:");
        LOG_INFO("Panels: " + std::to_string(panels_.size()));
        LOG_INFO("Pending docks: " + std::to_string(pendingDockUpdates_.size()));
        LOG_INFO("Dockspace ID: " + std::to_string(dockspaceId_));
        for (size_t i = 0; i < dockRegionIds_.size(); ++i)
            LOG_INFO("Region[" + std::to_string(i) + "] ID: " + std::to_string(dockRegionIds_[i]));
        LOG_INFO("───────────────────────────────");
    }

    // ────────────────────────────────────────────────
    // 🧩 Fonctions internes
    // ────────────────────────────────────────────────

    void GuiSystem::ApplyDockingPreferences_()
    {
        if (!dockLayoutBuilt_ || dockspaceId_ == 0 || pendingDockUpdates_.empty())
            return;

        std::vector<GuiPanel*> panelsToProcess;
        panelsToProcess.reserve(pendingDockUpdates_.size());

        if (useSavedDockLayout_)
        {
            for (GuiPanel* panel : pendingDockUpdates_)
            {
                if (!panel)
                    continue;

                const auto& title = panel->GetTitle();
                const auto& name = panel->GetName();

                ImGuiWindowSettings* settings = nullptr;
                if (!title.IsEmpty())
                {
                    const ImGuiID seed = name.IsEmpty() ? 0 : ImHashStr(name.c_str());
                    const ImGuiID windowId = ImHashStr(title.c_str(), 0, seed);
                    settings = ImGui::FindWindowSettingsByID(windowId);
                }

                if (settings && settings->DockId != 0)
                    continue;

                panelsToProcess.push_back(panel);
            }
        }
        else
        {
            for (GuiPanel* panel : pendingDockUpdates_)
            {
                if (panel)
                    panelsToProcess.push_back(panel);
            }
        }

        if (panelsToProcess.empty())
        {
            pendingDockUpdates_.clear();
            return;
        }

        std::vector<GuiPanel*> remaining;
        remaining.reserve(panelsToProcess.size());

        for (GuiPanel* panel : panelsToProcess)
        {
            if (!panel)
                continue;

            DockSpaceRegion region = panel->HasDockingPreference() ? panel->GetDockingPreference() : DockSpaceRegion::Center;

            std::size_t index = static_cast<std::size_t>(region);
            if (index >= dockRegionIds_.size())
                index = static_cast<std::size_t>(DockSpaceRegion::Center);

            ImGuiID dockId = dockRegionIds_[index];
            if (dockId == 0)
                dockId = dockRegionIds_[static_cast<std::size_t>(DockSpaceRegion::Center)];

            if (dockId == 0)
            {
                remaining.push_back(panel);
                continue;
            }

            ImGuiCond fallbackCondition = panel->HasDockingPreference()
                ? panel->GetDockingPreferenceCondition()
                : ImGuiCond_FirstUseEver;

            panel->ResetDockId();
            panel->SetDockId(dockId, ImGuiCond_Always, fallbackCondition);
        }

        pendingDockUpdates_ = std::move(remaining);
    }

    void GuiSystem::QueuePanelForDockUpdate_(GuiPanel& panel)
    {
        if (std::find(pendingDockUpdates_.begin(), pendingDockUpdates_.end(), &panel) == pendingDockUpdates_.end())
            pendingDockUpdates_.push_back(&panel);
    }

    void GuiSystem::RemovePanelFromDockQueue_(GuiPanel& panel)
    {
        auto it = std::find(pendingDockUpdates_.begin(), pendingDockUpdates_.end(), &panel);
        if (it != pendingDockUpdates_.end())
            pendingDockUpdates_.erase(it);
    }

    void GuiSystem::QueueAllPanelsForDockUpdate_()
    {
        if (useSavedDockLayout_)
            return;

        for (GuiPanel* panel : panels_)
        {
            if (panel)
                QueuePanelForDockUpdate_(*panel);
        }
    }

    bool GuiSystem::HasSavedDockLayout_() const
    {
        if (!ImGui::GetCurrentContext())
            return false;

        const ImGuiIO& io = ImGui::GetIO();
        if (!io.IniFilename || *io.IniFilename == '\0')
            return false;

        std::error_code error;
        if (!std::filesystem::exists(io.IniFilename, error) || error)
            return false;

        std::ifstream file(io.IniFilename);
        if (!file.is_open())
            return false;

        std::string line;
        bool inDockingSection = false;
        while (std::getline(file, line))
        {
            if (line.rfind("[Docking][Data]", 0) == 0)
            {
                inDockingSection = true;
                continue;
            }

            if (!inDockingSection)
                continue;

            if (!line.empty() && line.front() == '[')
                break;

            if (line.find("DockSpace") != std::string::npos)
                return true;
        }

        return false;
    }

}

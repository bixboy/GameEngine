#include "Engine/Gui/GuiSystem.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>

#include <SDL3/SDL.h>

#include "Core/Logger.h"
#include "Engine/Gui/GuiPanel.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "imgui_internal.h"

namespace BixEngine::Gui
{
    GuiSystem::~GuiSystem()
    {
        Shutdown();
    }

    bool GuiSystem::Initialize(SDL_Window* window, SDL_Renderer* renderer)
    {
        if (initialized_)
            return true;

        if (!window || !renderer)
        {
            LOG_ERROR("GuiSystem initialization failed: invalid window or renderer.");
            return false;
        }

        window_ = window;
        renderer_ = renderer;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        dockingEnabled_ = (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) != 0;

        ImGui::StyleColorsDark();

        useSavedDockLayout_ = dockingEnabled_ && HasSavedDockLayout_();

        if (dockingEnabled_)
        {
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowMinSize = ImVec2(150.0f, 120.0f);
        }

        if (!ImGui_ImplSDL3_InitForSDLRenderer(window_, renderer_))
        {
            const char* error = SDL_GetError();
            if (error && *error)
                LOG_ERROR(String{"Failed to initialize ImGui SDL3 backend: "} + error);
            else
                LOG_ERROR("Failed to initialize ImGui SDL3 backend.");

            ImGui::DestroyContext();
            window_ = nullptr;
            renderer_ = nullptr;
            return false;
        }

        if (!ImGui_ImplSDLRenderer3_Init(renderer_))
        {
            const char* error = SDL_GetError();
            if (error && *error)
                LOG_ERROR(String{"Failed to initialize ImGui SDL renderer backend: "} + error);
            else
                LOG_ERROR("Failed to initialize ImGui SDL renderer backend.");

            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();
            window_ = nullptr;
            renderer_ = nullptr;
            return false;
        }

        initialized_ = true;
        dockLayoutBuilt_ = useSavedDockLayout_;
        rebuildDockLayout_ = !useSavedDockLayout_;
        dockRegionIds_.fill(0);
        pendingDockUpdates_.clear();
        return true;
    }

    void GuiSystem::Shutdown() noexcept
    {
        if (!initialized_)
            return;

        panels_.clear();
        pendingDockUpdates_.clear();

        if (ImGuiContext* context = ImGui::GetCurrentContext())
        {
            ImGui::SetCurrentContext(context);
            ImGui::DestroyPlatformWindows();

            ImGuiIO& io = ImGui::GetIO();
            if (io.IniFilename && *io.IniFilename)
                ImGui::SaveIniSettingsToDisk(io.IniFilename);
        }

        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();

        if (ImGuiContext* context = ImGui::GetCurrentContext())
            ImGui::DestroyContext(context);

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
    }

    void GuiSystem::BeginFrame()
    {
        if (!initialized_)
            return;

        if (!ImGui::GetCurrentContext())
        {
            LOG_WARNING("ImGui context not available; disabling GUI rendering.");
            Shutdown();
            return;
        }

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
        if (!initialized_ || !frameBegun_)
            return;

        ImGui::EndFrame();
        frameBegun_ = false;
    }

    void GuiSystem::Render()
    {
        if (!initialized_)
            return;

        if (frameBegun_)
        {
            ImGui::EndFrame();
            frameBegun_ = false;
        }

        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer_);
    }

    void GuiSystem::ProcessEvent(const SDL_Event& event)
    {
        if (!initialized_)
            return;

        ImGui_ImplSDL3_ProcessEvent(&event);
    }

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
        auto it = std::find(panels_.begin(), panels_.end(), &panel);
        if (it != panels_.end())
            panels_.erase(it);

        RemovePanelFromDockQueue_(panel);
    }

    void GuiSystem::EnqueueDockUpdate(GuiPanel& panel)
    {
        if (!dockingEnabled_)
            return;

        QueuePanelForDockUpdate_(panel);
    }

    ImGuiID GuiSystem::GetRegionDockId(DockSpaceRegion region) const noexcept
    {
        const std::size_t index = static_cast<std::size_t>(region);
        if (index >= dockRegionIds_.size())
            return 0;

        return dockRegionIds_[index];
    }

    void GuiSystem::RequestDockLayoutRebuild() noexcept
    {
        rebuildDockLayout_ = true;
        dockLayoutBuilt_ = false;
        dockRegionIds_.fill(0);
        useSavedDockLayout_ = false;
        QueueAllPanelsForDockUpdate_();
    }

    void GuiSystem::BeginDockspaceLayout_()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        if (!viewport)
            return;

        const ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar;
        const ImGuiWindowFlags windowFlags =
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoBackground;

        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("EngineDockSpace", nullptr, windowFlags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspaceId = ImGui::GetID("EngineDockSpace::DockSpace");
        dockspaceId_ = dockspaceId;

        if (rebuildDockLayout_)
        {
            BuildDefaultDockLayout_(*viewport, dockspaceId, dockspaceFlags);
            rebuildDockLayout_ = false;
        }

        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);

        ImGui::End();
    }

    void GuiSystem::BuildDefaultDockLayout_(ImGuiViewport& viewport, ImGuiID dockspaceId, ImGuiDockNodeFlags dockspaceFlags)
    {
        if (dockspaceId == 0)
            return;

        dockRegionIds_.fill(0);

        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, dockspaceFlags | ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodePos(dockspaceId, viewport.WorkPos);
        ImGui::DockBuilderSetNodeSize(dockspaceId, viewport.WorkSize);

        ImGuiID dockMainId = dockspaceId;
        ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Left, 0.20f, nullptr, &dockMainId);
        ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Right, 0.25f, nullptr, &dockMainId);
        ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Down, 0.25f, nullptr, &dockMainId);
        ImGuiID dockTop = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Up, 0.15f, nullptr, &dockMainId);

        dockRegionIds_[static_cast<std::size_t>(DockSpaceRegion::Center)] = dockMainId;
        dockRegionIds_[static_cast<std::size_t>(DockSpaceRegion::Left)] = dockLeft;
        dockRegionIds_[static_cast<std::size_t>(DockSpaceRegion::Right)] = dockRight;
        dockRegionIds_[static_cast<std::size_t>(DockSpaceRegion::Bottom)] = dockBottom;
        dockRegionIds_[static_cast<std::size_t>(DockSpaceRegion::Top)] = dockTop;

        ImGui::DockBuilderFinish(dockspaceId);

        dockLayoutBuilt_ = true;
        QueueAllPanelsForDockUpdate_();
    }

    void GuiSystem::ApplyDockingPreferences_()
    {
        if (!dockLayoutBuilt_ || dockspaceId_ == 0 || pendingDockUpdates_.empty())
            return;

        if (useSavedDockLayout_)
        {
            pendingDockUpdates_.clear();
            return;
        }

        std::vector<GuiPanel*> remaining;
        remaining.reserve(pendingDockUpdates_.size());

        for (GuiPanel* panel : pendingDockUpdates_)
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
        if (useSavedDockLayout_)
            return;

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

#include "Gui/GuiSystem.h"

#include <algorithm>
#include <string>
#include <utility>

#include <SDL3/SDL.h>

#include "Core/Logger.h"
#include "Gui/GuiPanel.h"
#include "Gui/LayoutSystem.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

namespace Engine::Gui
{
    GuiSystem::GuiSystem()
        : layoutSystem_(std::make_unique<LayoutSystem>())
    {
    }

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

#ifdef IMGUI_HAS_DOCK
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#else
        LOG_WARNING("ImGui was built without docking support; layout docking features are disabled.");
#endif

        ImGui::StyleColorsDark();

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
        return true;
    }

    void GuiSystem::Shutdown() noexcept
    {
        if (!initialized_)
            return;

        panels_.clear();

        if (layoutSystem_)
            layoutSystem_->Clear();

        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();

        if (ImGuiContext* context = ImGui::GetCurrentContext())
            ImGui::DestroyContext(context);

        initialized_ = false;
        frameBegun_ = false;
        window_ = nullptr;
        renderer_ = nullptr;
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
            panels_.push_back(&panel);

        if (layoutSystem_)
            layoutSystem_->RegisterPanel(&panel);
    }

    void GuiSystem::UnregisterPanel(GuiPanel& panel)
    {
        auto it = std::find(panels_.begin(), panels_.end(), &panel);
        if (it != panels_.end())
            panels_.erase(it);

        if (layoutSystem_)
            layoutSystem_->UnregisterPanel(&panel);
    }
}

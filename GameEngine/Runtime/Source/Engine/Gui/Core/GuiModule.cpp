#include "Engine/Gui/Core/GuiModule.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <filesystem>
#include <functional>
#include <span>
#include <utility>
#include <vector>
#include <imgui_internal.h>

#include "Core/Logger.h"
#include "Core/Containers/String.h"
#include "Engine/Systems/SubsystemManager.h"
#include "Engine/Systems/Window.h"
#include "Engine/Gui/DefaultEngineGui.h"
#include "Engine/Gui/GuiContextFactory.h"
#include "Engine/Gui/Core/GuiManager.h"
#include "Engine/Gui/Core/GuiLayoutManager.h"
#include "Engine/Gui/Core/GuiPanel.h"
#include "Engine/Gui/Core/GuiSystem.h"

#include "Game/Actor.h"
#include "Graphics/Renderer.h"

namespace BixEngine::Core
{
    namespace
    {
        constexpr float kNavigationBarHeight = 38.0f;
    }

    GuiModule::GuiModule() = default;

    GuiModule::~GuiModule() noexcept
    {
        Shutdown();
    }

    bool GuiModule::Initialize(Window& window, Graphics::Renderer& renderer)
    {
        if (bInitialized_)
            return true;

        if (!guiSystem_)
            guiSystem_ = std::make_unique<Gui::GuiSystem>();

        if (!guiSystem_->Initialize(window.GetSDLWindow(), renderer.GetSDLRenderer()))
        {
            guiSystem_->Shutdown();
            guiSystem_.reset();
            guiManager_.reset();
            layoutManager_.reset();
            bInitialized_ = false;
            return false;
        }

        guiManager_ = std::make_unique<Gui::GuiManager>(*guiSystem_);
        layoutManager_ = std::make_unique<Gui::GuiLayoutManager>(*guiSystem_, *guiManager_);
        DestroySceneViewportTexture();

        auto focusWindow = [this](const std::string& windowName)
        {
            focusRequests_.push_back(windowName);
        };

        auto focusScene = [this]()
        {
            FocusSceneViewport();
        };

        if (!actorEditorManager_)
        {
            actorEditorManager_ = std::make_unique<GuiActorEditorManager>(*guiManager_, layoutManager_.get(),
                                                                          focusWindow, focusScene);
        }
        else
        {
            actorEditorManager_->SetLayoutManager(layoutManager_.get());
            actorEditorManager_->SetFocusCallbacks(focusWindow, focusScene);
        }

        if (actorEditorManager_)
        {
            actorEditorManager_->SetSubsystems(subsystems_);
            actorEditorManager_->ActivateScene(false);
        }

        focusRequests_.clear();
        bInitialized_ = true;
        return true;
    }

    void GuiModule::Shutdown() noexcept
    {
        if (!bInitialized_ && !guiSystem_ && !guiManager_ && !layoutManager_)
            return;

        statsPanel_ = nullptr;
        outlinerPanel_ = nullptr;
        contentBrowserPanel_ = nullptr;
        inspectorPanel_ = nullptr;
        viewportPanel_ = nullptr;
        selectedActor_ = nullptr;
        lastDeltaTime_ = nullptr;

        if (layoutManager_)
        {
            layoutManager_->SaveCurrentLayout();
            layoutManager_->SaveAllLayoutsToDisk();
        }

        if (actorEditorManager_)
            actorEditorManager_->RemoveAllEditors();

        focusRequests_.clear();
        subsystems_ = nullptr;

        DestroySceneViewportTexture();
        actorEditorManager_.reset();
        if (layoutManager_)
            layoutManager_.reset();
        guiManager_.reset();

        if (guiSystem_)
        {
            guiSystem_->Shutdown();
            guiSystem_.reset();
        }

        bInitialized_ = false;
    }

    bool GuiModule::IsInitialized() const noexcept
    {
        return bInitialized_ && guiSystem_ && guiSystem_->IsInitialized();
    }

    bool GuiModule::ProcessEvent(const SDL_Event& event)
    {
        if (!IsInitialized() || !guiSystem_)
            return false;

        guiSystem_->ProcessEvent(event);

        const ImGuiIO& io = ImGui::GetIO();
        const bool overViewport = IsMouseOverViewport();

        switch (event.type)
        {
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            
            case SDL_EVENT_MOUSE_BUTTON_UP:
            
            case SDL_EVENT_MOUSE_MOTION:
            
            case SDL_EVENT_MOUSE_WHEEL:
                return io.WantCaptureMouse && !overViewport;

            case SDL_EVENT_KEY_DOWN:
            
            case SDL_EVENT_KEY_UP:
            
            case SDL_EVENT_TEXT_INPUT:
                return io.WantCaptureKeyboard && !overViewport;

        default:
            return false;
        }
    }

    void GuiModule::BeginFrame()
    {
        if (!IsInitialized())
            return;

        if (guiSystem_)
            guiSystem_->SetDockspaceTopPadding(kNavigationBarHeight);

        if (layoutManager_)
            layoutManager_->Render();

        guiSystem_->BeginFrame();
    }

    void GuiModule::Render(SubsystemManager& subsystems)
    {
        if (!IsInitialized())
            return;

        subsystems_ = &subsystems;
        if (actorEditorManager_)
            actorEditorManager_->SetSubsystems(subsystems_);

        if (guiManager_)
        {
            guiManager_->DrawAll();
            ProcessFocusRequests();
        }

        guiSystem_->EndFrame();
        guiSystem_->Render();
    }

    void GuiModule::SetupDefaultGuiPanels(SubsystemManager& subsystems, const float* lastDeltaTimePointer)
    {
        lastDeltaTime_ = lastDeltaTimePointer;
        subsystems_ = &subsystems;
        if (actorEditorManager_)
            actorEditorManager_->SetSubsystems(subsystems_);

        if (!guiManager_)
        {
            statsPanel_ = nullptr;
            outlinerPanel_ = nullptr;
            contentBrowserPanel_ = nullptr;
            inspectorPanel_ = nullptr;
            viewportPanel_ = nullptr;
            selectedActor_ = nullptr;
            focusRequests_.clear();
            if (actorEditorManager_)
                actorEditorManager_->RemoveAllEditors();
            return;
        }

        if (actorEditorManager_)
            actorEditorManager_->RemoveAllEditors();

        focusRequests_.clear();
        selectedActor_ = nullptr;

        Gui::DefaultEngineGuiContextFactory contextFactory(subsystems);

        Gui::DefaultEngineGuiContextArgs contextArgs{
            subsystems,
            lastDeltaTime_,
            &selectedActor_,
            &sceneViewportTexture_,
            [this]() -> std::pair<int, int>
            {
                return {sceneViewportWidth_, sceneViewportHeight_};
            }
        };

        contextArgs.openScriptFilesInEditor = [](const std::vector<std::filesystem::path>& paths)
        {
            if (paths.empty())
                return;

            String message = "Requested to open script files in code editor:";
            for (const auto& path : paths)
            {
                message += "- ";
                message += path.generic_string();
            }

            LOG_INFO(message);
        };

        contextArgs.openActorInEditor = [this](const std::filesystem::path& actorPath)
        {
            OpenActorEditor(actorPath);
        };

        const Gui::DefaultEngineGuiContext context = contextFactory.CreateContext(contextArgs);
        const Gui::DefaultEngineGuiPanels panels = Gui::CreateDefaultEngineGui(*guiManager_, context);
        viewportPanel_ = panels.sceneViewportPanel;
        statsPanel_ = panels.statsPanel;
        outlinerPanel_ = panels.sceneOutlinerPanel;
        contentBrowserPanel_ = panels.contentBrowserPanel;
        inspectorPanel_ = panels.actorInspectorPanel;

        if (layoutManager_)
        {
            std::array<Gui::GuiPanel*, 5> scenePanelBuffer{};
            std::size_t count = 0;
            auto pushScenePanel = [&](Gui::GuiPanel* panel)
            {
                if (panel)
                    scenePanelBuffer[count++] = panel;
            };

            pushScenePanel(viewportPanel_);
            pushScenePanel(outlinerPanel_);
            pushScenePanel(contentBrowserPanel_);
            pushScenePanel(inspectorPanel_);
            pushScenePanel(statsPanel_);

            const std::span panelsSpan(scenePanelBuffer.data(), count);
            layoutManager_->RegisterPanels(Gui::EditorLayoutType::Scene, panelsSpan,
                                           Gui::GuiLayoutManager::LayoutRegistrationMode::ForceLoad);
        }

        if (actorEditorManager_)
        {
            actorEditorManager_->ActivateScene(false);
            actorEditorManager_->RefreshActorPanelsVisibility();
        }
    }

    bool GuiModule::IsMouseOverViewport() const noexcept
    {
        if (!viewportPanel_)
            return false;

        const ImVec2 mousePos = ImGui::GetMousePos();

        const ImVec2 windowPos = viewportPanel_->GetPosition();
        const ImVec2 windowSize = viewportPanel_->GetSize();

        return (
            mousePos.x >= windowPos.x && mousePos.x <= windowPos.x + windowSize.x &&
            mousePos.y >= windowPos.y && mousePos.y <= windowPos.y + windowSize.y
        );
    }

    void GuiModule::OpenActorEditor(const std::filesystem::path& path)
    {
        if (!actorEditorManager_)
            return;

        actorEditorManager_->SetSubsystems(subsystems_);
        actorEditorManager_->OpenActorEditor(path);
    }

    void GuiModule::CloseActorEditor(const std::string& navigationId)
    {
        if (!actorEditorManager_)
            return;

        actorEditorManager_->CloseActorEditor(navigationId);
    }

    void GuiModule::ProcessFocusRequests()
    {
        if (focusRequests_.empty())
            return;

        std::vector<std::string> remainingRequests;
        remainingRequests.reserve(focusRequests_.size());

        for (const std::string& windowName : focusRequests_)
        {
            ImGuiWindow* window = ImGui::FindWindowByName(windowName.c_str());
            if (!window || !window->Active)
            {
                remainingRequests.push_back(windowName);
                continue;
            }

            ImGui::SetWindowFocus(windowName.c_str());
        }

        focusRequests_ = std::move(remainingRequests);
    }

    void GuiModule::FocusSceneViewport()
    {
        if (!viewportPanel_)
            return;

        viewportPanel_->SetVisible(true);
        focusRequests_.push_back(viewportPanel_->GetTitle().Std());
    }

    bool GuiModule::EnsureSceneViewportTexture(Graphics::Renderer& renderer)
    {
        if (!IsInitialized())
        {
            DestroySceneViewportTexture();
            return false;
        }

        SDL_Renderer* sdlRenderer = renderer.GetSDLRenderer();
        if (!sdlRenderer)
        {
            DestroySceneViewportTexture();
            return false;
        }

        int outputWidth = 0;
        int outputHeight = 0;
        if (!SDL_GetCurrentRenderOutputSize(sdlRenderer, &outputWidth, &outputHeight) || outputWidth <= 0 || outputHeight <= 0)
        {
            DestroySceneViewportTexture();
            return false;
        }

        if (sceneViewportTexture_ && (outputWidth != sceneViewportWidth_ || outputHeight != sceneViewportHeight_))
            DestroySceneViewportTexture();

        if (!sceneViewportTexture_)
        {
            sceneViewportTexture_ = SDL_CreateTexture(
                sdlRenderer,
                SDL_PIXELFORMAT_RGBA32,
                SDL_TEXTUREACCESS_TARGET,
                outputWidth,
                outputHeight);

            if (!sceneViewportTexture_)
            {
                if (!sceneViewportTextureErrorLogged_)
                {
                    LOG_ERROR(String{"Failed to create scene viewport texture: "} + SDL_GetError());
                    sceneViewportTextureErrorLogged_ = true;
                }

                sceneViewportWidth_ = 0;
                sceneViewportHeight_ = 0;
                return false;
            }

            SDL_SetTextureBlendMode(sceneViewportTexture_, SDL_BLENDMODE_BLEND);
            sceneViewportWidth_ = outputWidth;
            sceneViewportHeight_ = outputHeight;
            sceneViewportTextureErrorLogged_ = false;
        }

        return sceneViewportTexture_ != nullptr;
    }

    void GuiModule::DestroySceneViewportTexture() noexcept
    {
        if (sceneViewportTexture_)
        {
            SDL_DestroyTexture(sceneViewportTexture_);
            sceneViewportTexture_ = nullptr;
        }

        sceneViewportWidth_ = 0;
        sceneViewportHeight_ = 0;
        sceneViewportTextureErrorLogged_ = false;
    }
}

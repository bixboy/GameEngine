#include "Engine/Gui/Internal/GuiModule.h"

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
#include "Engine/Gui/Internal/GuiManager.h"
#include "Engine/Gui/Internal/GuiLayoutManager.h"
#include "Engine/Gui/Internal/GuiPanel.h"
#include "Engine/Gui/Internal/GuiSystem.h"
#include "Engine/Gui/Internal/NavBar/GuiNavigationBar.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserPanel.h"
#include "Input/Input.h"

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
        navigationBar_ = std::make_unique<GuiNavigationBar>(*guiSystem_, *layoutManager_, *this);
        DestroySceneViewportTexture();

        auto focusWindow = [this](const std::string& windowName)
        {
            focusRequests_.push_back(windowName);
        };

        auto focusScene = [this]()
        {
            FocusSceneViewport();
        };

        if (!assetEditorManager_)
        {
            assetEditorManager_ = std::make_unique<GuiAssetEditorManager>(*guiManager_, layoutManager_.get(),
                                                                         focusWindow, focusScene);
        }
        else
        {
            assetEditorManager_->SetLayoutManager(layoutManager_.get());
            assetEditorManager_->SetFocusCallbacks(focusWindow, focusScene);
        }

        if (assetEditorManager_)
            assetEditorManager_->ActivateScene(false);

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

        if (assetEditorManager_)
        {
            // Reset any asset editors before rebuilding the default GUI panels to avoid
            // keeping stale ImGui panel handles after a project or layout reload.
            assetEditorManager_->RemoveAllEditors();
        }

        focusRequests_.clear();
        subsystems_ = nullptr;

        DestroySceneViewportTexture();
        navigationBar_.reset();
        assetEditorManager_.reset();
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
            case SDL_EVENT_DROP_FILE:
            {
                bool handled = false;
                if (auto* browser = Gui::ContentBrowserPanel::GetActiveInstance())
                {
                    if (event.drop.data && *event.drop.data)
                    {
                        const std::filesystem::path droppedFile = event.drop.data;
                        browser->ImportExternalFiles({droppedFile});
                        handled = true;
                    }
                }

                if (event.drop.data)
                    SDL_free(const_cast<char*>(event.drop.data));

                return handled;
            }
            case SDL_EVENT_DROP_TEXT:
            case SDL_EVENT_DROP_COMPLETE:
            case SDL_EVENT_DROP_BEGIN:
            case SDL_EVENT_DROP_POSITION:
                if (event.drop.data)
                    SDL_free(const_cast<char*>(event.drop.data));
                return false;
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
        if (navigationBar_)
            navigationBar_->Render();

        if (guiManager_)
        {
            guiManager_->DrawAll();
            ProcessFocusRequests();
        }

        if (const Input::Input* input = subsystems.GetInputDevice())
        {
            const Input::MouseStatistics& stats = input->GetMouseStatistics();
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            const ImVec2 viewportPos = viewport ? viewport->WorkPos : ImVec2(0.0f, 0.0f);

            ImGui::SetNextWindowPos(ImVec2(viewportPos.x + 12.0f, viewportPos.y + 12.0f), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.35f);
            ImGui::SetNextWindowViewport(viewport ? viewport->ID : 0);

            const ImGuiWindowFlags overlayFlags =
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoDocking |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoInputs |
                ImGuiWindowFlags_NoNav |
                ImGuiWindowFlags_NoFocusOnAppearing |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_AlwaysAutoResize;

            if (ImGui::Begin("##MouseEventsOverlay", nullptr, overlayFlags))
            {
                ImGui::Text("Mouse events: %d/s", stats.eventsPerSecond);
                ImGui::Text("Dropped: %d/s", stats.droppedEventsPerSecond);
            }
            ImGui::End();
        }

        guiSystem_->EndFrame();
        guiSystem_->Render();
    }

    void GuiModule::SetupDefaultGuiPanels(SubsystemManager& subsystems, const float* lastDeltaTimePointer)
    {
        lastDeltaTime_ = lastDeltaTimePointer;
        subsystems_ = &subsystems;
        if (assetEditorManager_)
        {
            // Reset any asset editors before rebuilding the default GUI panels to avoid
            // keeping stale ImGui panel handles after a project or layout reload.
            assetEditorManager_->RemoveAllEditors();
        }

        if (!guiManager_)
        {
            statsPanel_ = nullptr;
            outlinerPanel_ = nullptr;
            contentBrowserPanel_ = nullptr;
            inspectorPanel_ = nullptr;
            viewportPanel_ = nullptr;
            selectedActor_ = nullptr;
            focusRequests_.clear();
            return;
        }

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

        contextArgs.openAssetInEditor = [this](const std::filesystem::path& assetPath)
        {
            OpenAssetEditor(assetPath);
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

        if (assetEditorManager_)
        {
            assetEditorManager_->ActivateScene(false);
            assetEditorManager_->RefreshAssetPanelsVisibility();
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

    void GuiModule::OpenAssetEditor(const std::filesystem::path& path)
    {
        if (!assetEditorManager_)
            return;

        assetEditorManager_->OpenAssetEditor(path);
    }

    void GuiModule::CloseAssetEditor(const std::string& navigationId)
    {
        if (!assetEditorManager_)
            return;

        assetEditorManager_->CloseAssetEditor(navigationId);
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

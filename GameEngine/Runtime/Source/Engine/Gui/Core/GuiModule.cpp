#include "Engine/Gui/Core/GuiModule.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <filesystem>
#include <functional>
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
#include "Engine/Gui/Panels/ActorEditorPanel.h"
#include "Engine/Gui/Controllers/ActorEditorController.h"
#include "Game/Actor.h"
#include "Graphics/Renderer.h"

namespace BixEngine::Core
{
    namespace
    {
        constexpr float kNavigationBarHeight = 38.0f;
    }

    GuiModule::GuiModule()
    {
        activeLayout_ = Gui::EditorLayoutType::Scene;
    }

    GuiModule::~GuiModule()
    {
        Shutdown();
    }

    bool GuiModule::Initialize(Window& window, Graphics::Renderer& renderer)
    {
        if (!guiSystem_)
            guiSystem_ = std::make_unique<Gui::GuiSystem>();

        if (!guiSystem_->Initialize(window.GetSDLWindow(), renderer.GetSDLRenderer()))
        {
            guiSystem_->Shutdown();
            guiSystem_.reset();
            guiManager_.reset();
            return false;
        }

        guiManager_ = std::make_unique<Gui::GuiManager>(*guiSystem_);
        layoutManager_ = std::make_unique<Gui::GuiLayoutManager>(*guiSystem_, *guiManager_);
        DestroySceneViewportTexture();
        return true;
    }

    void GuiModule::Shutdown() noexcept
    {
        statsPanel_ = nullptr;
        outlinerPanel_ = nullptr;
        contentBrowserPanel_ = nullptr;
        inspectorPanel_ = nullptr;
        viewportPanel_ = nullptr;
        selectedActor_ = nullptr;
        lastDeltaTime_ = nullptr;

        if (guiManager_)
        {
            for (auto& [_, entry] : actorEditors_)
            {
                if (entry.panel)
                    guiManager_->RemovePanel(entry.panel->GetName());
            }
        }

        actorEditors_.clear();
        focusRequests_.clear();
        activeNavigationId_ = "scene";
        subsystems_ = nullptr;

        DestroySceneViewportTexture();
        if (layoutManager_)
        {
            layoutManager_->SaveCurrentLayout();
            layoutManager_.reset();
        }
        guiManager_.reset();

        if (guiSystem_)
        {
            guiSystem_->Shutdown();
            guiSystem_.reset();
        }
    }

    bool GuiModule::IsInitialized() const noexcept
    {
        return guiSystem_ && guiSystem_->IsInitialized();
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

        if (guiManager_)
        {
            DrawEditorNavigation();
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

        if (!guiManager_)
        {
            statsPanel_ = nullptr;
            outlinerPanel_ = nullptr;
            contentBrowserPanel_ = nullptr;
            inspectorPanel_ = nullptr;
            viewportPanel_ = nullptr;
            selectedActor_ = nullptr;
            return;
        }

        if (!actorEditors_.empty())
        {
            for (auto& [_, entry] : actorEditors_)
            {
                if (guiManager_ && entry.panel)
                {
                    guiManager_->RemovePanel(entry.panel->GetName());
                    if (layoutManager_)
                        layoutManager_->RemovePanel(*entry.panel);
                }
            }
            actorEditors_.clear();
            focusRequests_.clear();
            activeNavigationId_ = "scene";
            activeLayout_ = Gui::EditorLayoutType::Scene;
        }

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
                message += "\n - ";
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
            std::vector<Gui::GuiPanel*> scenePanels;
            scenePanels.reserve(5);
            if (viewportPanel_) scenePanels.push_back(viewportPanel_);
            if (outlinerPanel_) scenePanels.push_back(outlinerPanel_);
            if (contentBrowserPanel_) scenePanels.push_back(contentBrowserPanel_);
            if (inspectorPanel_) scenePanels.push_back(inspectorPanel_);
            if (statsPanel_) scenePanels.push_back(statsPanel_);

            layoutManager_->SetPanelsForLayout(Gui::EditorLayoutType::Scene, scenePanels);
            layoutManager_->LoadLayout(Gui::EditorLayoutType::Scene);
        }

        activeLayout_ = Gui::EditorLayoutType::Scene;
        RefreshActorPanelsVisibility();
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
        if (!guiManager_ || path.empty())
            return;

        std::filesystem::path normalized = path.lexically_normal();
        std::string key = normalized.generic_string();
        if (key.empty())
            key = path.generic_string();

        if (key.empty())
            return;

        if (auto it = actorEditors_.find(key); it != actorEditors_.end())
        {
            ActorEditorEntry& entry = it->second;
            activeNavigationId_ = entry.navigationId;
            if (entry.panel)
            {
                entry.panel->SetVisible(true);
                focusRequests_.push_back(entry.panel->GetTitle().Std());
            }
            if (layoutManager_)
                layoutManager_->Switch(Gui::EditorLayoutType::ActorEditor);
            activeLayout_ = Gui::EditorLayoutType::ActorEditor;
            RefreshActorPanelsVisibility();
            return;
        }

        if (!subsystems_)
        {
            LOG_WARNING("[GuiModule] Unable to open actor editor without subsystem context.");
            return;
        }

        const std::string displayName = normalized.filename().empty()
            ? normalized.generic_string()
            : normalized.filename().generic_string();

        const std::string navigationId = key;
        const std::string panelName = "actor_editor_" + std::to_string(std::hash<std::string>{}(navigationId));

        Gui::ActorEditorPanel& actorPanel = guiManager_->CreatePanelOfType<Gui::ActorEditorPanel>(panelName, "Actor Editor");
        auto controller = std::make_unique<Gui::ActorEditorController>(*subsystems_, normalized, [this, navigationId]()
        {
            CloseActorEditor(navigationId);
        });

        Gui::GuiPanelController& attached = guiManager_->AttachController(actorPanel, std::move(controller));
        auto* editorController = static_cast<Gui::ActorEditorController*>(&attached);

        ActorEditorEntry entry{};
        entry.assetPath = normalized;
        entry.navigationId = navigationId;
        entry.tabLabel = "Actor: " + (displayName.empty() ? std::string("Untitled") : displayName);
        entry.panel = &actorPanel;
        entry.controller = editorController;

        if (layoutManager_)
            layoutManager_->AddPanel(Gui::EditorLayoutType::ActorEditor, actorPanel);

        actorEditors_.emplace(entry.navigationId, entry);

        actorPanel.SetVisible(true);
        activeNavigationId_ = entry.navigationId;
        focusRequests_.push_back(actorPanel.GetTitle().Std());
        if (layoutManager_)
            layoutManager_->Switch(Gui::EditorLayoutType::ActorEditor);
        activeLayout_ = Gui::EditorLayoutType::ActorEditor;
        RefreshActorPanelsVisibility();
    }

    void GuiModule::CloseActorEditor(const std::string& navigationId)
    {
        auto it = actorEditors_.find(navigationId);
        if (it == actorEditors_.end())
            return;

        if (guiManager_ && it->second.panel)
        {
            guiManager_->RemovePanel(it->second.panel->GetName());
            if (layoutManager_)
                layoutManager_->RemovePanel(*it->second.panel);
        }

        actorEditors_.erase(it);

        if (activeNavigationId_ == navigationId)
        {
            activeNavigationId_ = "scene";
            FocusSceneViewport();
            if (layoutManager_)
                layoutManager_->Switch(Gui::EditorLayoutType::Scene);
            activeLayout_ = Gui::EditorLayoutType::Scene;
        }

        RefreshActorPanelsVisibility();
    }

    void GuiModule::DrawEditorNavigation()
    {
        if (!guiSystem_ || !guiSystem_->IsInitialized())
            return;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        if (!viewport)
            return;

        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y));
        ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, kNavigationBarHeight));
        ImGui::SetNextWindowViewport(viewport->ID);

        const ImGuiWindowFlags flags =
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
                activeLayout_ = layoutManager_->GetCurrentLayout();

            const float buttonHeight = kNavigationBarHeight - 16.0f;
            auto drawLayoutButton = [&](const char* label, Gui::EditorLayoutType layoutType)
            {
                const bool isActive = activeLayout_ == layoutType;
                if (isActive)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.30f, 0.30f, 0.34f, 1.0f});
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.34f, 0.34f, 0.38f, 1.0f});
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.28f, 0.28f, 0.32f, 1.0f});
                }

                const bool clicked = ImGui::Button(label, ImVec2(120.0f, buttonHeight));

                if (isActive)
                    ImGui::PopStyleColor(3);

                return clicked;
            };

            if (drawLayoutButton("Scene", Gui::EditorLayoutType::Scene))
            {
                activeLayout_ = Gui::EditorLayoutType::Scene;
                activeNavigationId_ = "scene";
                if (layoutManager_)
                    layoutManager_->Switch(Gui::EditorLayoutType::Scene);
                RefreshActorPanelsVisibility();
                FocusSceneViewport();
            }

            ImGui::SameLine();

            if (drawLayoutButton("Actor Editor", Gui::EditorLayoutType::ActorEditor))
            {
                activeLayout_ = Gui::EditorLayoutType::ActorEditor;
                if (layoutManager_)
                    layoutManager_->Switch(Gui::EditorLayoutType::ActorEditor);

                if (activeNavigationId_ == "scene" && !actorEditors_.empty())
                {
                    std::vector<std::pair<std::string, std::string>> sortedEntries;
                    sortedEntries.reserve(actorEditors_.size());
                    for (const auto& [navigationId, entry] : actorEditors_)
                        sortedEntries.emplace_back(navigationId, entry.tabLabel);

                    std::sort(sortedEntries.begin(), sortedEntries.end(), [](const auto& lhs, const auto& rhs)
                    {
                        return lhs.second < rhs.second;
                    });

                    activeNavigationId_ = sortedEntries.front().first;
                    if (auto it = actorEditors_.find(activeNavigationId_); it != actorEditors_.end() && it->second.panel)
                        focusRequests_.push_back(it->second.panel->GetTitle().Std());
                }

                RefreshActorPanelsVisibility();
            }

            if (activeLayout_ == Gui::EditorLayoutType::ActorEditor)
            {
                ImGui::SameLine();

                if (actorEditors_.empty())
                {
                    ImGui::TextDisabled("No actor editor open");
                }
                else if (ImGui::BeginTabBar("ActorEditorTabs", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll))
                {
                    std::vector<std::pair<std::string, std::string>> sortedEntries;
                    sortedEntries.reserve(actorEditors_.size());
                    for (const auto& [navigationId, entry] : actorEditors_)
                        sortedEntries.emplace_back(navigationId, entry.tabLabel);

                    std::sort(sortedEntries.begin(), sortedEntries.end(), [](const auto& lhs, const auto& rhs)
                    {
                        return lhs.second < rhs.second;
                    });

                    for (const auto& [navigationId, tabLabel] : sortedEntries)
                    {
                        auto it = actorEditors_.find(navigationId);
                        if (it == actorEditors_.end())
                            continue;

                        ActorEditorEntry& entry = it->second;
                        ImGuiTabItemFlags tabFlags = ImGuiTabItemFlags_NoCloseWithMiddleMouseButton;
                        if (activeNavigationId_ == entry.navigationId)
                            tabFlags |= ImGuiTabItemFlags_SetSelected;

                        if (ImGui::BeginTabItem(tabLabel.c_str(), nullptr, tabFlags))
                        {
                            if (ImGui::IsItemActivated())
                            {
                                activeNavigationId_ = entry.navigationId;
                                if (layoutManager_)
                                    layoutManager_->Switch(Gui::EditorLayoutType::ActorEditor);

                                if (entry.panel)
                                    focusRequests_.push_back(entry.panel->GetTitle().Std());

                                RefreshActorPanelsVisibility();
                            }

                            ImGui::EndTabItem();
                        }
                    }

                    ImGui::EndTabBar();
                }
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

    void GuiModule::ProcessFocusRequests()
    {
        if (focusRequests_.empty())
            return;

        for (const std::string& windowName : focusRequests_)
            ImGui::SetWindowFocus(windowName.c_str());

        focusRequests_.clear();
    }

    void GuiModule::FocusSceneViewport()
    {
        if (!viewportPanel_)
            return;

        viewportPanel_->SetVisible(true);
        focusRequests_.push_back(viewportPanel_->GetTitle().Std());
    }

    void GuiModule::RefreshActorPanelsVisibility()
    {
        const bool actorLayoutActive = layoutManager_ && layoutManager_->GetCurrentLayout() == Gui::EditorLayoutType::ActorEditor;
        for (auto& [navigationId, entry] : actorEditors_)
        {
            if (!entry.panel)
                continue;

            const bool shouldBeVisible = actorLayoutActive && activeNavigationId_ == navigationId;
            entry.panel->SetVisible(shouldBeVisible);
        }
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

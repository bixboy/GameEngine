#include "Engine/Gui/Core/GuiModule.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <string_view>
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
#include "Engine/Gui/Controllers/ActorEditorController.h"
#include "Game/Actor.h"
#include "Graphics/Renderer.h"

namespace BixEngine::Core
{
    namespace
    {
        constexpr float kNavigationBarHeight = 38.0f;
        constexpr std::string_view kSceneNavigationId{"scene"};

        std::string MakeNavigationIdFromPath(const std::filesystem::path& path)
        {
            std::string raw = path.generic_string();
            if (raw.empty())
                raw = "actor";

            std::string sanitized;
            sanitized.reserve(raw.size());
            for (char ch : raw)
            {
                if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_')
                    sanitized.push_back(ch);
                else
                    sanitized.push_back('_');
            }

            constexpr std::uint64_t kFnvOffset = 1469598103934665603ull;
            constexpr std::uint64_t kFnvPrime = 1099511628211ull;
            std::uint64_t hash = kFnvOffset;
            for (unsigned char ch : raw)
            {
                hash ^= ch;
                hash *= kFnvPrime;
            }

            sanitized.append("_");
            sanitized.append(std::format("{:x}", hash));

            return "actor_editor_" + sanitized;
        }
    }

    GuiModule::GuiModule()
    {
        activeLayout_ = Gui::EditorLayoutType::Scene;
    }

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

        activeLayout_ = Gui::EditorLayoutType::Scene;
        activeNavigationId_ = std::string{kSceneNavigationId};
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

        if (guiManager_)
        {
            for (auto& [_, entry] : actorEditors_)
            {
                PanelBuffer buffer{};
                const auto panelSpan = CollectPanels(entry.panels, buffer);
                if (layoutManager_)
                    layoutManager_->DetachPanels(panelSpan);
                guiManager_->RemovePanels(panelSpan);
                entry.sharedState.reset();
            }
        }

        actorEditors_.clear();
        actorEditorsByPath_.clear();
        actorEditorOrder_.clear();
        focusRequests_.clear();
        activeNavigationId_ = std::string{kSceneNavigationId};
        activeLayout_ = Gui::EditorLayoutType::Scene;
        nextActorEditorId_ = 0;
        subsystems_ = nullptr;

        DestroySceneViewportTexture();
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
            actorEditors_.clear();
            actorEditorsByPath_.clear();
            actorEditorOrder_.clear();
            focusRequests_.clear();
            return;
        }

        if (!actorEditors_.empty())
        {
            for (auto& [_, entry] : actorEditors_)
            {
                PanelBuffer buffer{};
                const auto panelSpan = CollectPanels(entry.panels, buffer);
                if (layoutManager_)
                    layoutManager_->DetachPanels(panelSpan);
                guiManager_->RemovePanels(panelSpan);
                entry.sharedState.reset();
            }

            actorEditors_.clear();
            actorEditorsByPath_.clear();
            actorEditorOrder_.clear();
            nextActorEditorId_ = 0;
            focusRequests_.clear();
            activeNavigationId_ = std::string{kSceneNavigationId};
            activeLayout_ = Gui::EditorLayoutType::Scene;

            if (layoutManager_)
                layoutManager_->ResetLayout(Gui::EditorLayoutType::ActorEditor);
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
        if (normalized.empty())
            normalized = path;

        if (normalized.empty())
            return;

        if (!subsystems_)
        {
            LOG_WARNING("[GuiModule] Unable to open actor editor without subsystem context.");
            return;
        }

        auto existingByPath = actorEditorsByPath_.find(normalized);
        if (existingByPath != actorEditorsByPath_.end())
        {
            const std::string& navigationId = existingByPath->second;
            if (auto itEntry = actorEditors_.find(navigationId); itEntry != actorEditors_.end())
            {
                if (layoutManager_)
                    ApplyActorEditorPanels(itEntry->second);
            }
            activeNavigationId_ = navigationId;
            activeLayout_ = Gui::EditorLayoutType::ActorEditor;
            if (layoutManager_)
                layoutManager_->Switch(Gui::EditorLayoutType::ActorEditor);
            RefreshActorPanelsVisibility();

            if (auto itEntry = actorEditors_.find(navigationId); itEntry != actorEditors_.end())
            {
                if (itEntry->second.panels.viewport)
                    focusRequests_.push_back(itEntry->second.panels.viewport->GetTitle().Std());
            }
            return;
        }

        const std::string navigationId = MakeNavigationIdFromPath(normalized);
        const std::string baseName = navigationId;

        auto sharedState = Gui::ActorEditorController::CreateSharedState(*subsystems_, normalized,
                                                                         String(navigationId.c_str()),
                                                                         [this, navigationId]()
        {
            CloseActorEditor(navigationId);
        });

        if (!sharedState)
            return;

        ActorEditorEntry entry{};
        entry.assetPath = normalized;
        entry.navigationId = navigationId;
        entry.buttonLabel = sharedState->assetDisplayName.Std();
        entry.sharedState = sharedState;

        auto createPanel = [&](const std::string& suffix, Gui::ActorEditorController::Section section) -> Gui::GuiPanel*
        {
            const std::string panelId = baseName + "_" + suffix;
            Gui::GuiPanel& panel = guiManager_->CreatePanel(String(panelId.c_str()), String{"Actor Editor"});
            panel.SetVisible(false);
            auto controller = std::make_unique<Gui::ActorEditorController>(sharedState, section);
            guiManager_->AttachController(panel, std::move(controller));
            return &panel;
        };

        entry.panels.toolbar = createPanel("toolbar", Gui::ActorEditorController::Section::Toolbar);
        entry.panels.viewport = createPanel("viewport", Gui::ActorEditorController::Section::Viewport);
        entry.panels.outline = createPanel("outline", Gui::ActorEditorController::Section::Outline);
        entry.panels.inspector = createPanel("inspector", Gui::ActorEditorController::Section::Inspector);

        actorEditorOrder_.push_back(navigationId);
        actorEditorsByPath_.emplace(normalized, navigationId);
        auto [itInserted, inserted] = actorEditors_.emplace(navigationId, std::move(entry));
        static_cast<void>(inserted);
        ActorEditorEntry& storedEntry = itInserted->second;

        if (layoutManager_)
            ApplyActorEditorPanels(storedEntry);

        activeNavigationId_ = navigationId;
        if (layoutManager_)
            layoutManager_->Switch(Gui::EditorLayoutType::ActorEditor);
        activeLayout_ = Gui::EditorLayoutType::ActorEditor;
        RefreshActorPanelsVisibility();

        if (storedEntry.panels.viewport)
            focusRequests_.push_back(storedEntry.panels.viewport->GetTitle().Std());
    }

    void GuiModule::CloseActorEditor(const std::string& navigationId)
    {
        auto it = actorEditors_.find(navigationId);
        if (it == actorEditors_.end())
            return;

        ActorEditorEntry entry = std::move(it->second);
        actorEditors_.erase(it);

        size_t removedIndex = 0;
        bool removedFromOrder = false;
        if (!actorEditorOrder_.empty())
        {
            auto orderIt = std::find(actorEditorOrder_.begin(), actorEditorOrder_.end(), navigationId);
            if (orderIt != actorEditorOrder_.end())
            {
                removedIndex = static_cast<size_t>(std::distance(actorEditorOrder_.begin(), orderIt));
                actorEditorOrder_.erase(orderIt);
                removedFromOrder = true;
            }
        }

        PanelBuffer buffer{};
        const auto panelSpan = CollectPanels(entry.panels, buffer);
        if (layoutManager_)
            layoutManager_->DetachPanels(panelSpan);
        if (guiManager_)
            guiManager_->RemovePanels(panelSpan);

        actorEditorsByPath_.erase(entry.assetPath);

        entry.sharedState.reset();

        Gui::GuiPanel* panelToFocus = nullptr;
        bool focusScene = false;
        if (activeNavigationId_ == navigationId)
        {
            if (actorEditors_.empty())
            {
                activeNavigationId_ = std::string{kSceneNavigationId};
                focusScene = true;
                if (layoutManager_)
                {
                    layoutManager_->Switch(Gui::EditorLayoutType::Scene);
                    layoutManager_->ResetLayout(Gui::EditorLayoutType::ActorEditor);
                }
                activeLayout_ = Gui::EditorLayoutType::Scene;
            }
            else
            {
                if (actorEditorOrder_.empty())
                {
                    activeNavigationId_ = std::string{kSceneNavigationId};
                    focusScene = true;
                    if (layoutManager_)
                    {
                        layoutManager_->Switch(Gui::EditorLayoutType::Scene);
                        layoutManager_->ResetLayout(Gui::EditorLayoutType::ActorEditor);
                    }
                    activeLayout_ = Gui::EditorLayoutType::Scene;
                }
                else
                {
                    size_t nextIndex = 0;
                    if (removedFromOrder && removedIndex < actorEditorOrder_.size())
                        nextIndex = removedIndex;
                    else if (!actorEditorOrder_.empty())
                        nextIndex = actorEditorOrder_.size() - 1;

                    const std::string& nextNavigationId = actorEditorOrder_[nextIndex];
                    activeNavigationId_ = nextNavigationId;
                    activeLayout_ = Gui::EditorLayoutType::ActorEditor;
                    if (layoutManager_)
                        layoutManager_->Switch(Gui::EditorLayoutType::ActorEditor);
                    if (auto nextIt = actorEditors_.find(nextNavigationId); nextIt != actorEditors_.end())
                    {
                        ApplyActorEditorPanels(nextIt->second);
                        panelToFocus = nextIt->second.panels.viewport;
                    }
                }
            }
        }

        if (actorEditors_.empty() && layoutManager_)
            layoutManager_->ResetLayout(Gui::EditorLayoutType::ActorEditor);

        RefreshActorPanelsVisibility();

        if (focusScene)
            FocusSceneViewport();
        else if (panelToFocus)
            focusRequests_.push_back(panelToFocus->GetTitle().Std());
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
            // TODO(workspaces): Replace this navigation row with a workspace-aware toolbar when multi-workspace support lands.

            DrawSceneButton(buttonHeight);
            DrawActorEditorTabs(buttonHeight);
        }

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

    bool GuiModule::DrawNavigationButton(const std::string& label, bool isActive, float buttonHeight) const
    {
        if (isActive)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.30f, 0.30f, 0.34f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.34f, 0.34f, 0.38f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.28f, 0.28f, 0.32f, 1.0f});
        }

        const ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        const float paddingX = 32.0f;
        const ImVec2 size{textSize.x + paddingX, buttonHeight};
        const bool clicked = ImGui::Button(label.c_str(), size);

        if (isActive)
            ImGui::PopStyleColor(3);

        return clicked;
    }

    bool GuiModule::DrawCloseButton(std::string_view label, float buttonHeight) const
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

    void GuiModule::DrawSceneButton(float buttonHeight)
    {
        static const std::string kSceneLabel{"Scene"};
        const bool sceneActive = activeNavigationId_ == kSceneNavigationId || actorEditors_.empty();
        if (!DrawNavigationButton(kSceneLabel, sceneActive, buttonHeight))
            return;

        activeNavigationId_ = std::string{kSceneNavigationId};
        activeLayout_ = Gui::EditorLayoutType::Scene;
        if (layoutManager_)
            layoutManager_->Switch(Gui::EditorLayoutType::Scene);
        RefreshActorPanelsVisibility();
        FocusSceneViewport();
    }

    void GuiModule::DrawActorEditorTabs(float buttonHeight)
    {
        if (actorEditorOrder_.empty())
            return;

        std::vector<std::string> closeRequests;
        std::vector<std::string> staleEntries;
        closeRequests.reserve(actorEditorOrder_.size());
        staleEntries.reserve(actorEditorOrder_.size());

        for (const std::string& navId : actorEditorOrder_)
        {
            auto entryIt = actorEditors_.find(navId);
            if (entryIt == actorEditors_.end())
            {
                staleEntries.push_back(navId);
                continue;
            }

            ActorEditorEntry& entry = entryIt->second;
            if (entry.sharedState)
            {
                const std::string displayName = entry.sharedState->assetDisplayName.Std();
                if (entry.buttonLabel != displayName)
                    entry.buttonLabel = displayName;
            }

            ImGui::SameLine();
            const bool isActive = activeNavigationId_ == entry.navigationId;

            ImGui::PushID(entry.navigationId.c_str());

            const std::string& label = entry.buttonLabel.empty() ? entry.navigationId : entry.buttonLabel;
            if (DrawNavigationButton(label, isActive, buttonHeight))
            {
                activeNavigationId_ = entry.navigationId;
                activeLayout_ = Gui::EditorLayoutType::ActorEditor;
                if (layoutManager_)
                {
                    ApplyActorEditorPanels(entry);
                    layoutManager_->Switch(Gui::EditorLayoutType::ActorEditor);
                }
                RefreshActorPanelsVisibility();
                if (entry.panels.viewport)
                    focusRequests_.push_back(entry.panels.viewport->GetTitle().Std());
            }

            ImGui::SameLine(0.0f, 6.0f);
            if (DrawCloseButton(label, buttonHeight))
                closeRequests.push_back(entry.navigationId);

            ImGui::PopID();
        }

        for (const std::string& stale : staleEntries)
        {
            auto orderIt = std::find(actorEditorOrder_.begin(), actorEditorOrder_.end(), stale);
            if (orderIt != actorEditorOrder_.end())
                actorEditorOrder_.erase(orderIt);
        }

        for (const std::string& navigationId : closeRequests)
            CloseActorEditor(navigationId);
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

    void GuiModule::RefreshActorPanelsVisibility()
    {
        const bool actorLayoutActive =
            (layoutManager_ && layoutManager_->GetCurrentLayout() == Gui::EditorLayoutType::ActorEditor) ||
            activeLayout_ == Gui::EditorLayoutType::ActorEditor;
        for (auto& [navigationId, entry] : actorEditors_)
        {
            const bool shouldBeVisible = actorLayoutActive && activeNavigationId_ == navigationId;
            entry.panels.ForEachPanel([shouldBeVisible](Gui::GuiPanel* panel)
            {
                panel->SetVisible(shouldBeVisible);
            });
        }
    }

    void GuiModule::ApplyActorEditorPanels(ActorEditorEntry& entry)
    {
        if (!layoutManager_)
            return;

        PanelBuffer buffer{};
        const auto panelSpan = CollectPanels(entry.panels, buffer);
        layoutManager_->RegisterPanels(Gui::EditorLayoutType::ActorEditor, panelSpan,
                                       Gui::GuiLayoutManager::LayoutRegistrationMode::LoadIfUninitialized);
    }


    std::span<Gui::GuiPanel*> GuiModule::CollectPanels(const ActorEditorPanels& panels, PanelBuffer& buffer) const noexcept
    {
        std::span spanBuffer{buffer};
        return panels.CopyTo(spanBuffer);
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

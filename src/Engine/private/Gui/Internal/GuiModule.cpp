#include "Gui/Internal/GuiModule.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <filesystem>
#include <functional>
#include <span>
#include <string>
#include <vector>
#include <imgui_internal.h>

#include "Debug/Logger.h"
#include "Containers/String.h"
#include "Framework/Actor.h"
#include "Renderer.h"
#include "Serializer/SceneSerializer.h"
#include "Framework/Scene.h"

#include "Gui/Core/DefaultEngineGui.h"
#include "Gui/Core/GuiContextFactory.h"
#include "Gui/Core/GuiManager.h"
#include "Gui/Internal/GuiLayoutManager.h"
#include "Gui/Internal/GuiSystem.h"
#include "Gui/Internal/NavBar/GuiNavigationBar.h"
#include "Gui/Internal/EditorSceneManager.h"
#include "Gui/Internal/MainMenuBar.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserPanel.h"
#include "Gui/Panels/GuiPanel.h"


namespace BixEngine::Gui
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

    // --- Initialization ---

    bool GuiModule::Initialize(Core::Window& window, Graphics::Renderer& renderer)
    {
        if (bInitialized_)
            return true;

        if (!guiSystem_)
            guiSystem_ = std::make_unique<Gui::GuiSystem>();
        
        if (!guiSystem_->Initialize(window.GetSDLWindow(), renderer.GetSDLRenderer()))
        {
            guiSystem_->Shutdown();
            guiSystem_.reset();
            return false;
        }

        guiManager_ = std::make_unique<GuiManager>(*guiSystem_);
        layoutManager_ = std::make_unique<GuiLayoutManager>(*guiSystem_, *guiManager_);
        editorSceneManager_ = std::make_unique<EditorSceneManager>();
        mainMenuBar_ = std::make_unique<MainMenuBar>(*guiManager_, *layoutManager_, *editorSceneManager_);
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

        assetEditorManager_ = std::make_unique<GuiAssetEditorManager>(*guiManager_, layoutManager_.get(), focusWindow, focusScene);
        
        if (assetEditorManager_)
            assetEditorManager_->ActivateScene(false);

        focusRequests_.clear();
        bInitialized_ = true;
        return true;
    }

    void GuiModule::Shutdown() noexcept
    {
        if (!bInitialized_ && !guiSystem_) return;

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
            assetEditorManager_->RemoveAllEditors();
            assetEditorManager_.reset();
        }

        focusRequests_.clear();
        subsystems_ = nullptr;

        DestroySceneViewportTexture();
        navigationBar_.reset();
        
        if (layoutManager_)
            layoutManager_.reset();
        
        if (guiManager_)
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

    // --- Events & Frame ---

    bool GuiModule::ProcessEvent(const SDL_Event& event)
    {
        if (!IsInitialized() || !guiSystem_) return false;

        guiSystem_->ProcessEvent(event);

        const ImGuiIO& io = ImGui::GetIO();
        const bool overViewport = IsMouseOverViewport();

        switch (event.type)
        {
        case SDL_EVENT_DROP_FILE:
            {
                std::filesystem::path droppedFile{};
                if (event.drop.data && *event.drop.data)
                {
                    const std::string dropString(event.drop.data);
                    std::u8string dropUtf8;
                    dropUtf8.reserve(dropString.size());
                    
                    for (const unsigned char ch : dropString)
                    {
                        dropUtf8.push_back(static_cast<char8_t>(ch));
                    }
                    
                    droppedFile = std::filesystem::path(dropUtf8);
                }

                if (!droppedFile.empty())
                    pendingDroppedFiles_.push_back(std::move(droppedFile));

                return true;
            }
        case SDL_EVENT_MOUSE_WHEEL:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            return io.WantCaptureMouse && !overViewport;
            
        case SDL_EVENT_TEXT_INPUT:
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            return io.WantCaptureKeyboard && !overViewport;

        default:
            return false;
        }
    }

    void GuiModule::DispatchPendingFileDrops()
    {
        if (pendingDroppedFiles_.empty())
            return;

        if (auto* browser = ContentBrowserPanel::GetActiveInstance())
        {
            browser->ImportExternalFiles(pendingDroppedFiles_);
            pendingDroppedFiles_.clear();
        }
    }

    void GuiModule::BeginFrame()
    {
        if (!IsInitialized())
            return;

        if (guiSystem_)
            guiSystem_->SetDockspaceTopPadding(kNavigationBarHeight);
        
        if (layoutManager_)
            layoutManager_->Update();

        guiSystem_->BeginFrame();
        
        if (mainMenuBar_)
            mainMenuBar_->Draw();
            
        if (layoutManager_)
            layoutManager_->Render();
            
        if (editorSceneManager_)
            editorSceneManager_->DrawDialogs();
    }

    void GuiModule::Render(Core::SubsystemManager& subsystems)
    {
        if (!IsInitialized())
            return;

        subsystems_ = &subsystems;
        DispatchPendingFileDrops();

        if (navigationBar_)
            navigationBar_->Render();

        if (guiManager_)
        {
            guiManager_->DrawAll();
            ProcessFocusRequests();
        }

        guiSystem_->EndFrame();
        guiSystem_->Render();
    }

    // --- SETUP PRINCIPAL (Updated) ---

    void GuiModule::SetupDefaultGuiPanels(Core::SubsystemManager& subsystems, const float* lastDeltaTimePointer)
    {
        lastDeltaTime_ = lastDeltaTimePointer;
        subsystems_ = &subsystems;

        if (assetEditorManager_)
            assetEditorManager_->RemoveAllEditors();

        if (!guiManager_)
        {
            viewportPanel_ = nullptr;
            selectedActor_ = nullptr;
            focusRequests_.clear();
            return;
        }

        focusRequests_.clear();
        selectedActor_ = nullptr;
        
        DefaultEngineGuiContextArgs contextArgs{};
        
        contextArgs.lastDeltaTime = lastDeltaTime_;
        contextArgs.selectedActorSlot = &selectedActor_;
        contextArgs.sceneViewportTexture = &sceneViewportTexture_;
        
        contextArgs.sceneViewportSizeProvider = [this]() -> std::pair<int, int>
        {
            return {sceneViewportWidth_, sceneViewportHeight_};
        };

        DefaultEngineGuiContextFactory contextFactory(subsystems);
        DefaultEngineGuiContext context = contextFactory.CreateContext(contextArgs);
        
        context.openAssetInEditor = [this](const std::filesystem::path& path)
        {
            this->OpenAssetEditor(path);
        };        
        
        SetupDefaultEditorBehaviors(*guiManager_, context);

        const DefaultEngineGuiPanels panels = CreateDefaultEngineGui(*guiManager_, context);
        
        viewportPanel_ = panels.sceneViewportPanel;

        if (layoutManager_)
        {
            std::array<GuiPanel*, 5> scenePanelBuffer{};
            std::size_t count = 0;
            auto pushScenePanel = [&](GuiPanel* panel)
            {
                if (panel)
                    scenePanelBuffer[count++] = panel;
            };

            pushScenePanel(viewportPanel_);
            pushScenePanel(panels.sceneOutlinerPanel);
            pushScenePanel(panels.contentBrowserPanel);
            pushScenePanel(panels.actorInspectorPanel);
            pushScenePanel(panels.statsPanel);

            const std::span panelsSpan(scenePanelBuffer.data(), count);
            layoutManager_->RegisterPanels(DefaultLayouts::Scene, panelsSpan, GuiLayoutManager::LayoutRegistrationMode::ForceLoad);
        }

        if (assetEditorManager_)
        {
            assetEditorManager_->ActivateScene(false);
            assetEditorManager_->RefreshAssetPanelsVisibility();
        }
    }

    // --- Viewport Utilities ---

    bool GuiModule::IsMouseOverViewport() const noexcept
    {
        if (!viewportPanel_)
            return false;

        const ImVec2 mousePos = ImGui::GetMousePos();
        const ImVec2 windowPos = viewportPanel_->GetPosition();
        const ImVec2 windowSize = viewportPanel_->GetSize();

        return (mousePos.x >= windowPos.x && mousePos.x <= windowPos.x + windowSize.x &&
            mousePos.y >= windowPos.y && mousePos.y <= windowPos.y + windowSize.y);
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

        int outputWidth = 0, outputHeight = 0;
        if (!SDL_GetCurrentRenderOutputSize(sdlRenderer, &outputWidth, &outputHeight) || outputWidth <= 0 || outputHeight <= 0)
        {
            DestroySceneViewportTexture();
            return false;
        }

        if (sceneViewportTexture_ && (outputWidth != sceneViewportWidth_ || outputHeight != sceneViewportHeight_))
            DestroySceneViewportTexture();

        if (!sceneViewportTexture_)
        {
            sceneViewportTexture_ = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, outputWidth, outputHeight);

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

    // --- Helpers ---

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

    // --- Play/Stop/Pause ---

    void GuiModule::OnPlay()
    {
        if (m_EngineState != EngineState::Edit)
            return;
        
        if (!subsystems_)
            return;

        Game::Scene* activeScene = subsystems_->GetActiveScene();
        if (!activeScene)
            return;
        
        m_SceneBackup.str(""); 
        m_SceneBackup.clear();
        Serialization::SceneSerializer::SerializeBinary(*activeScene, m_SceneBackup);

        m_EngineState = EngineState::Play;
        activeScene->OnRuntimeStart();
    }

    void GuiModule::OnStop()
    {
        if (m_EngineState == EngineState::Edit)
            return;
        
        if (!subsystems_)
            return;

        Game::Scene* activeScene = subsystems_->GetActiveScene();
        if (!activeScene)
            return;

        selectedActor_ = nullptr;
        subsystems_->ResetInput();
        
        activeScene->OnRuntimeStop();

        m_SceneBackup.clear(); 
        m_SceneBackup.seekg(0, std::ios::beg);
        
        if (!Serialization::SceneSerializer::DeserializeBinary(*activeScene, m_SceneBackup))
        {
            LOG_ERROR("Failed to restore scene from backup during OnStop()");
        }
        
        m_EngineState = EngineState::Edit;
    }

    void GuiModule::OnPause()
    {
        if (m_EngineState == EngineState::Edit)
            return;
        
        m_EngineState = (m_EngineState == EngineState::Play) ? EngineState::Pause : EngineState::Play;
    }
}
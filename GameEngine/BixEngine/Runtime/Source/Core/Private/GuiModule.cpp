#include "Bix/Core/GuiModule.h"

#include <SDL3/SDL.h>

#include "Bix/Core/Logger.h"
#include "Bix/Core/String.h"
#include "Bix/Core/SubsystemManager.h"
#include "Bix/Core/Timer.h"
#include "Bix/Core/Window.h"
#include "Bix/Engine/Gui/DefaultEngineGui.h"
#include "Bix/Engine/Gui/GuiManager.h"
#include "Bix/Engine/Gui/GuiPanel.h"
#include "Bix/Engine/Gui/GuiSystem.h"
#include "Bix/Game/Actor.h"
#include "Bix/Game/SceneManager.h"
#include "Bix/Graphics/Renderer.h"

namespace BixEngine::Core
{
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

        DestroySceneViewportTexture();
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

    void GuiModule::ProcessEvent(const SDL_Event& event)
    {
        if (IsInitialized())
            guiSystem_->ProcessEvent(event);
    }

    void GuiModule::BeginFrame()
    {
        if (IsInitialized())
            guiSystem_->BeginFrame();
    }

    void GuiModule::Render(SubsystemManager& subsystems)
    {
        if (!IsInitialized())
            return;

        if (guiManager_)
            guiManager_->DrawAll();

        guiSystem_->EndFrame();
        guiSystem_->Render();
    }

    void GuiModule::SetupDefaultGuiPanels(SubsystemManager& subsystems, const float* lastDeltaTimePointer)
    {
        lastDeltaTime_ = lastDeltaTimePointer;

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

        selectedActor_ = nullptr;

        const Gui::DefaultEngineGuiContext context{
            subsystems.GetTimer(),
            [&subsystems]() -> Game::SceneManager*
            {
                return subsystems.GetSceneManager();
            },
            lastDeltaTime_,
            [this]() -> Game::Actor*
            {
                return selectedActor_;
            },
            [this](Game::Actor* actor)
            {
                selectedActor_ = actor;
            },
            [this]() -> SDL_Texture*
            {
                return sceneViewportTexture_;
            },
            [this]() -> std::pair<int, int>
            {
                return {sceneViewportWidth_, sceneViewportHeight_};
            }
        };

        const Gui::DefaultEngineGuiPanels panels = Gui::CreateDefaultEngineGui(*guiManager_, context);
        viewportPanel_ = panels.sceneViewportPanel;
        statsPanel_ = panels.statsPanel;
        outlinerPanel_ = panels.sceneOutlinerPanel;
        contentBrowserPanel_ = panels.contentBrowserPanel;
        inspectorPanel_ = panels.actorInspectorPanel;
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

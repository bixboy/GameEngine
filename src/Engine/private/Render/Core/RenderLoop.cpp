#include "Render/Core/RenderLoop.h"
#include "Gui/Internal/GuiModule.h"
#include "Systems/Core/SubsystemManager.h"
#include "Time/Timer.h"
#include "Framework/Scene.h"
#include "Renderer.h"

namespace BixEngine::Core
{
    void RenderLoop::Configure(SubsystemManager* subsystems, Gui::GuiModule* guiModule, Graphics::Renderer* renderer, Math::Color clearColor) noexcept
    {
        subsystems_ = subsystems;
        guiModule_ = guiModule;
        renderer_ = renderer;
        clearColor_ = clearColor;
    }

    void RenderLoop::Reset() noexcept
    {
        subsystems_ = nullptr;
        guiModule_ = nullptr;
        renderer_ = nullptr;
        clearColor_ = {0, 0, 0, 255};
        lastDeltaTime_ = 0.0f;
    }

    float RenderLoop::CalculateDeltaTime()
    {
        lastDeltaTime_ = 0.0f;

        if (!subsystems_)
            return lastDeltaTime_;

        if (Timer* timer = subsystems_->GetTimer())
        {
            timer->Tick();
            lastDeltaTime_ = timer->GetDeltaTime();
        }

        return lastDeltaTime_;
    }

    void RenderLoop::BeginFrame()
    {
        if (guiModule_)
            guiModule_->BeginFrame();
    }

    void RenderLoop::Update(float deltaTime)
    {
        if (!subsystems_)
            return;

        if (!guiModule_)
        {
            subsystems_->UpdateRuntime(deltaTime);
            return;
        }

        switch (guiModule_->GetEngineState())
        {
        case Gui::GuiModule::EngineState::Edit:
            subsystems_->UpdateEditor(deltaTime);
            break;
            
        case Gui::GuiModule::EngineState::Play:
            subsystems_->UpdateRuntime(deltaTime);
            break;
            
        case Gui::GuiModule::EngineState::Pause:
            subsystems_->UpdatePaused(deltaTime);
            break;
            
        case Gui::GuiModule::EngineState::Step:
            subsystems_->UpdateRuntime(deltaTime);
            guiModule_->OnPause(); 
            break;
        }
    }
    
    void RenderLoop::Render()
    {
        if (!renderer_)
            return;

        Game::Scene* activeScene = subsystems_ ? subsystems_->GetActiveScene() : nullptr;
        SDL_Renderer* sdlRenderer = renderer_->GetSDLRenderer();
        
        const bool renderedToTexture = guiModule_ && guiModule_->EnsureSceneViewportTexture(*renderer_);
        SDL_Texture* viewportTexture = guiModule_ ? guiModule_->GetSceneViewportTexture() : nullptr;

        if (renderedToTexture && sdlRenderer && viewportTexture)
        {
            SDL_SetRenderTarget(sdlRenderer, viewportTexture);
            renderer_->Clear(clearColor_);

            if (activeScene)
            {
                activeScene->Render(*renderer_);
                activeScene->PostRender(*renderer_);
            }

            SDL_SetRenderTarget(sdlRenderer, nullptr);
            
            renderer_->Clear(clearColor_); 
        }
        else
        {
            renderer_->Clear(clearColor_);

            if (activeScene)
            {
                activeScene->Render(*renderer_);
                activeScene->PostRender(*renderer_);
            }
        }
        
        if (guiModule_ && subsystems_)
        {
            guiModule_->Render(*subsystems_);
        }

        renderer_->Present();
    }
}
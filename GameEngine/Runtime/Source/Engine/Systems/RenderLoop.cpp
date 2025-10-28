#include "Engine/Systems/RenderLoop.h"
#include "Engine/Gui/Core/GuiModule.h"
#include "Engine/Systems/SubsystemManager.h"
#include "Core/Timer.h"
#include "Game/Scene.h"
#include "Graphics/Renderer.h"

namespace BixEngine::Core
{
    void RenderLoop::Configure(SubsystemManager* subsystems, GuiModule* guiModule, Graphics::Renderer* renderer, Math::Color clearColor) noexcept
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
        if (subsystems_)
            subsystems_->UpdateAll(deltaTime);
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
                activeScene->Render(*renderer_);

            SDL_SetRenderTarget(sdlRenderer, nullptr);
            renderer_->Clear(clearColor_);
        }
        else
        {
            renderer_->Clear(clearColor_);

            if (activeScene)
                activeScene->Render(*renderer_);
        }

        if (guiModule_ && subsystems_)
            guiModule_->Render(*subsystems_);

        renderer_->Present();
    }
}

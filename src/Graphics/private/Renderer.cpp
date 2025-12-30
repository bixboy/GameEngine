#include "Renderer.h"
#include "Debug/Logger.h"
#include "Containers/String.h"


namespace BixEngine::Graphics
{
    Renderer::Renderer(SDL_Window* window, const char* driverName, bool useVSync)
    {
        if (!window)
        {
            LOG_ERROR("Renderer creation failed: invalid SDL_Window pointer.");
            return;
        }

        renderer_ = SDL_CreateRenderer(window, driverName);
        if (!renderer_)
        {
            LOG_ERROR(String{"Renderer creation failed: "} + SDL_GetError());
            return;
        }

        instance_ = this;

        SetVSync(useVSync);
        UpdateViewport();
        
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    }

    Renderer::~Renderer()
    {
        if (instance_ == this)
            instance_ = nullptr;

        if (renderer_)
        {
            SDL_DestroyRenderer(renderer_);
            renderer_ = nullptr;
        }
    }

    void Renderer::Clear(Math::Color color) const noexcept
    {
        if (!renderer_)
            return;
        
        SDL_SetRenderDrawColorFloat(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderClear(renderer_);
    }

    void Renderer::Present() const noexcept
    {
        if (renderer_) 
            SDL_RenderPresent(renderer_);
    }

    void Renderer::SetDrawColor(const Math::Color& color) const noexcept
    {
        if (renderer_)
            SDL_SetRenderDrawColorFloat(renderer_, color.r, color.g, color.b, color.a);
    }

    void Renderer::SetDrawColor(float r, float g, float b, float a) const noexcept
    {
        if (renderer_)
            SDL_SetRenderDrawColorFloat(renderer_, r, g, b, a);
    }

    void Renderer::SetVSync(bool enabled) const noexcept
    {
        if (renderer_)
        {
            // SDL3: 1 = VSync, 0 = No VSync, 2 = Adaptive
            SDL_SetRenderVSync(renderer_, enabled ? 1 : 0);
        }
    }

    void Renderer::OnResize(int width, int height)
    {
        UpdateViewport();
    }

    void Renderer::UpdateViewport() const noexcept
    {
        if (!renderer_)
            return;

        int width = 0, height = 0;
        if (!SDL_GetCurrentRenderOutputSize(renderer_, &width, &height))
            return;

        SDL_Rect viewport{0, 0, width, height};
        SDL_SetRenderViewport(renderer_, &viewport);
    }

    void Renderer::SetLogicalSize(int width, int height) 
    {
        if (renderer_ && width > 0 && height > 0)
        {
            logicalWidth_ = width;
            logicalHeight_ = height;
            SDL_SetRenderLogicalPresentation(renderer_, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
        }
        else
        {
            logicalWidth_ = 0;
            logicalHeight_ = 0;
            SDL_SetRenderLogicalPresentation(renderer_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
            UpdateViewport();
        }
    }
}
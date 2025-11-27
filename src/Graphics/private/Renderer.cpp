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

        if (useVSync)
            SDL_SetRenderVSync(renderer_, 1);
        else
            SDL_SetRenderVSync(renderer_, 0);

        UpdateViewport();

        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    }

    Renderer::~Renderer()
    {
        instance_ = nullptr;
        if (renderer_)
        {
            SDL_DestroyRenderer(renderer_);
            renderer_ = nullptr;
        }
    }

    void Renderer::Clear(Math::Color color) const noexcept
    {
        if (!renderer_) return;
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderClear(renderer_);
    }

    void Renderer::Present() const noexcept
    {
        if (renderer_) SDL_RenderPresent(renderer_);
    }

    void Renderer::SetColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) const noexcept
    {
        if (renderer_) SDL_SetRenderDrawColor(renderer_, r, g, b, a);
    }

    void Renderer::UpdateViewport() const noexcept
    {
        if (!renderer_) return;

        int width = 0, height = 0;
        if (!SDL_GetCurrentRenderOutputSize(renderer_, &width, &height))
            return;

        SDL_Rect viewport{0, 0, width, height};
        SDL_SetRenderViewport(renderer_, &viewport);
    }

    void Renderer::SetLogicalSize(int width, int height) const noexcept
    {
        if (renderer_ && width > 0 && height > 0)
            SDL_SetRenderLogicalPresentation(renderer_, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    }
}

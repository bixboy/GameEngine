#include "Graphics/Renderer.h"

#include <string>

#include "Core/Logger.h"

namespace Engine::Graphics {

namespace {
constexpr Uint32 kRendererFlags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
}

Renderer::Renderer(SDL_Window* window) {
    renderer_ = SDL_CreateRenderer(window, nullptr, kRendererFlags);

    if (!renderer_) {
        LOG_ERROR(std::string{"Renderer creation failed: "} + SDL_GetError());
    }
}

Renderer::~Renderer() {
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
}

void Renderer::Clear(SDL_Color color) const {
    if (!renderer_) {
        return;
    }

    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderClear(renderer_);
}

void Renderer::Present() const {
    if (renderer_) {
        SDL_RenderPresent(renderer_);
    }
}

void Renderer::SetColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) const {
    if (renderer_) {
        SDL_SetRenderDrawColor(renderer_, r, g, b, a);
    }
}

}  // namespace Engine::Graphics

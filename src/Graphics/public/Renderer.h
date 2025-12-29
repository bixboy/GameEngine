#pragma once
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include "Math/Color.h"

namespace BixEngine::Graphics
{
     
    class Renderer
    {
    public:
         
        explicit Renderer(SDL_Window* window, const char* driverName = nullptr, bool useVSync = false);

        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) noexcept = delete;
        Renderer& operator=(Renderer&&) noexcept = delete;

        static Renderer* Get() noexcept { return instance_; }

        [[nodiscard]] bool IsValid() const noexcept { return renderer_ != nullptr; }

         
        void Clear(Math::Color color) const noexcept;

         
        void Present() const noexcept;

         
        void SetColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) const noexcept;

         
        void UpdateViewport() const noexcept;

         
        void SetLogicalSize(int width, int height) const noexcept;

        [[nodiscard]] SDL_Renderer* GetSDLRenderer() const noexcept { return renderer_; }

    private:
        static inline Renderer* instance_ = nullptr;
        SDL_Renderer* renderer_{nullptr};
    };
}

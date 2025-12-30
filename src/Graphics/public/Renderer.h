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

        static Renderer* Get() noexcept { return instance_; }
        
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) noexcept = delete;
        Renderer& operator=(Renderer&&) noexcept = delete;

        [[nodiscard]] bool IsValid() const noexcept { return renderer_ != nullptr; }

        // --- Core Rendering ---

        void Clear(Math::Color color) const noexcept;
        void Present() const noexcept;

        // --- État du Renderer ---

        void SetDrawColor(const Math::Color& color) const noexcept;
        void SetDrawColor(float r, float g, float b, float a) const noexcept;

        void SetVSync(bool enabled) const noexcept;

        // --- Gestion de la Taille (Windowing) ---

        void OnResize(int width, int height);

        void UpdateViewport() const noexcept;

        void SetLogicalSize(int width, int height);
        
        [[nodiscard]] SDL_Renderer* GetSDLRenderer() const noexcept { return renderer_; }

    private:
        static inline Renderer* instance_ = nullptr;
        SDL_Renderer* renderer_{nullptr};
        
        int logicalWidth_{0};
        int logicalHeight_{0};
    };
}
#pragma once
#include <string>
#include <SDL3/SDL_pixels.h>

namespace Engine::Math
{
    struct Color
    {
        float r{1.0f};
        float g{1.0f};
        float b{1.0f};
        float a{1.0f};

        constexpr Color() = default;
        constexpr Color(float red, float green, float blue, float alpha = 1.0f)
            : r(red), g(green), b(blue), a(alpha) {}

        explicit Color(const SDL_Color& sdlColor)
            : r(static_cast<float>(sdlColor.r) / 255.0f),
              g(static_cast<float>(sdlColor.g) / 255.0f),
              b(static_cast<float>(sdlColor.b) / 255.0f),
              a(static_cast<float>(sdlColor.a) / 255.0f) {}

        [[nodiscard]] SDL_Color ToSDL() const noexcept
        {
            return SDL_Color{
                static_cast<Uint8>(r * 255.0f),
                static_cast<Uint8>(g * 255.0f),
                static_cast<Uint8>(b * 255.0f),
                static_cast<Uint8>(a * 255.0f)
            };
        }

        [[nodiscard]] static constexpr Color White()  { return Color(1.0f, 1.0f, 1.0f); }
        [[nodiscard]] static constexpr Color Black()  { return Color(0.0f, 0.0f, 0.0f); }
        [[nodiscard]] static constexpr Color Red()    { return Color(1.0f, 0.0f, 0.0f); }
        [[nodiscard]] static constexpr Color Green()  { return Color(0.0f, 1.0f, 0.0f); }
        [[nodiscard]] static constexpr Color Blue()   { return Color(0.0f, 0.0f, 1.0f); }
        [[nodiscard]] static constexpr Color Yellow() { return Color(1.0f, 1.0f, 0.0f); }

        [[nodiscard]] static Color Lerp(const Color& c1, const Color& c2, float t)
        {
            return Color(
                c1.r + (c2.r - c1.r) * t,
                c1.g + (c2.g - c1.g) * t,
                c1.b + (c2.b - c1.b) * t,
                c1.a + (c2.a - c1.a) * t
            );
        }

        [[nodiscard]] std::string ToString() const
        {
            return "Color(R=" + std::to_string(r) +
                   ", G=" + std::to_string(g) +
                   ", B=" + std::to_string(b) +
                   ", A=" + std::to_string(a) + ")";
        }
    };
}
#pragma once
#include <SDL3/SDL_pixels.h>
#include <algorithm>
#include "Containers/String.h"


namespace BixEngine::Math
{
    struct Color
    {
        float r{1.0f};
        float g{1.0f};
        float b{1.0f};
        float a{1.0f};

        constexpr Color() = default;

        constexpr Color(float red, float green, float blue, float alpha = 1.0f) : r(red), g(green), b(blue), a(alpha)
        {}

        explicit Color(const SDL_Color& sdlColor) :
            r(static_cast<float>(sdlColor.r) / 255.0f),
            g(static_cast<float>(sdlColor.g) / 255.0f),
            b(static_cast<float>(sdlColor.b) / 255.0f),
            a(static_cast<float>(sdlColor.a) / 255.0f)
        {}

        static constexpr Color FromHex(uint32_t hexValue)
        {
            return Color(
                static_cast<float>   ((hexValue >> 24) & 0xFF) / 255.0f,
                static_cast<float> ((hexValue >> 16) & 0xFF) / 255.0f,
                static_cast<float>  ((hexValue >> 8) & 0xFF) / 255.0f,
                static_cast<float> ((hexValue) & 0xFF) / 255.0f
            );
        }

        // --- Conversion vers SDL ---
        
        [[nodiscard]] SDL_Color ToSDL() const noexcept
        {
            return SDL_Color
            {
                static_cast<Uint8>(std::clamp(r, 0.0f, 1.0f) * 255.0f),
                static_cast<Uint8>(std::clamp(g, 0.0f, 1.0f) * 255.0f),
                static_cast<Uint8>(std::clamp(b, 0.0f, 1.0f) * 255.0f),
                static_cast<Uint8>(std::clamp(a, 0.0f, 1.0f) * 255.0f)
            };
        }

        // --- Couleurs ---
        
        [[nodiscard]] static constexpr Color White()   { return {1.0f, 1.0f, 1.0f}; }
        [[nodiscard]] static constexpr Color Black()   { return {0.0f, 0.0f, 0.0f}; }
        [[nodiscard]] static constexpr Color Red()     { return {1.0f, 0.0f, 0.0f}; }
        [[nodiscard]] static constexpr Color Green()   { return {0.0f, 1.0f, 0.0f}; }
        [[nodiscard]] static constexpr Color Blue()    { return {0.0f, 0.0f, 1.0f}; }
        [[nodiscard]] static constexpr Color Yellow()  { return {1.0f, 1.0f, 0.0f}; }
        [[nodiscard]] static constexpr Color Cyan()    { return {0.0f, 1.0f, 1.0f}; }
        [[nodiscard]] static constexpr Color Magenta() { return {1.0f, 0.0f, 1.0f}; }
        [[nodiscard]] static constexpr Color Gray()    { return {0.5f, 0.5f, 0.5f}; }
        
        [[nodiscard]] static constexpr Color Transparent() { return {0.0f, 0.0f, 0.0f, 0.0f}; }

        // --- Opérations Mathématiques ---

        // Addition (Lumière additive)
        Color operator+(const Color& other) const
        {
            return {r + other.r, g + other.g, b + other.b, a + other.a};
        }

        // Multiplication par Scalaire (Intensité / Assombrir)
        Color operator*(float scalar) const
        {
            return {r * scalar, g * scalar, b * scalar, a * scalar};
        }

        // Multiplication Color * Color (Tint / Filtre)
        Color operator*(const Color& other) const
        {
            return {r * other.r, g * other.g, b * other.b, a * other.a};
        }

        bool operator==(const Color& other) const
        {
            constexpr float epsilon = 1e-5f; 

            return std::abs(r - other.r) < epsilon &&
                   std::abs(g - other.g) < epsilon &&
                   std::abs(b - other.b) < epsilon &&
                   std::abs(a - other.a) < epsilon;
        }

        bool operator!=(const Color& other) const
        {
            return !(*this == other);
        }

        // --- Utilitaires ---

        // Interpolation
        [[nodiscard]] static Color Lerp(const Color& c1, const Color& c2, float t)
        {
            t = std::clamp(t, 0.0f, 1.0f);
            
            return {
                c1.r   + (c2.r - c1.r) * t,
                c1.g + (c2.g - c1.g) * t,
                c1.b  + (c2.b - c1.b) * t,
                c1.a + (c2.a - c1.a) * t
            };
        }

        [[nodiscard]] float Grayscale() const
        {
            return 0.299f * r + 0.587f * g + 0.114f * b;
        }

        [[nodiscard]] String ToString() const
        {
            return String("Color(") + std::to_string(r) + ", " + std::to_string(g) + ", " + std::to_string(b) + ", " + std::to_string(a) + ")";
        }
    };
}
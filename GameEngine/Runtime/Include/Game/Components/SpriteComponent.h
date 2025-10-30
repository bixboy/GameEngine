#pragma once

#include "Game/Components/Component.h"
#include "Reflection/BixReflection.h"
#include "SDL3/SDL.h"
#include "Core/Math/Rect.h"
#include "Core/Math/Vector2.h"
#include "SpriteComponent.generated.h"

namespace BixEngine::Render
{
    class Texture;
}

namespace BixEngine::Game
{
    class SpriteComponent;

    BCLASS()
    class SpriteComponent : public Component
    {
        GENERATED_BODY()

    public:
        /**
         * @brief Creates a sprite component using default dimensions and color.
         */
        explicit SpriteComponent(Actor* owner);

        SpriteComponent(Actor* owner, SDL_Color color, float w, float h) : Component(owner), color_(color), width_(w), height_(h) {}

        /** Renders the sprite to the provided renderer. */
        void Render(Graphics::Renderer& renderer) const override;

        /** Sets the base color of the sprite. */
        void SetColor(SDL_Color color) noexcept { color_ = color; }
        /** Sets the multiplicative tint applied to the texture. */
        void SetTint(SDL_Color tint) noexcept { tint_ = tint; }
        /** Sets the additive tint that brightens the sprite. */
        void SetAdditiveTint(SDL_Color tint) noexcept { additiveTint_ = tint; }
        /** Sets the emission color used by custom materials. */
        void SetEmissionColor(SDL_Color color) noexcept { emissionColor_ = color; }

        /** Updates the sprite dimensions in world units. */
        void SetDimensions(float w, float h) noexcept
        {
            width_ = w;
            height_ = h;
        }

        /** Assigns the SDL texture used for rendering. */
        void SetTexture(Render::Texture* texture) noexcept;
        /** Assigns the UV rectangle to sample from the texture. */
        void SetUVRect(const Math::Rect& uvRect) noexcept;

        /** Enables or disables horizontal flipping. */
        void SetFlipX(bool enabled) noexcept { bFlipX_ = enabled; }
        /** Enables or disables vertical flipping. */
        void SetFlipY(bool enabled) noexcept { bFlipY_ = enabled; }
        /** Sets the pivot/origin used for rendering. Values are expressed in normalized coordinates. */
        void SetPivot(const Math::Vector2<float>& pivot) noexcept { pivot_ = pivot; }
        /** Assigns the material/shader identifier for advanced renderers. */
        void SetMaterialId(String materialId) noexcept { materialId_ = std::move(materialId); }
        /** Overrides the blend mode used to composite the texture. */
        void SetBlendMode(SDL_BlendMode mode) noexcept { blendMode_ = mode; }

        [[nodiscard]] SDL_Color GetColor() const noexcept { return color_; }
        [[nodiscard]] SDL_Color GetTint() const noexcept { return tint_; }
        [[nodiscard]] SDL_Color GetAdditiveTint() const noexcept { return additiveTint_; }
        [[nodiscard]] SDL_Color GetEmissionColor() const noexcept { return emissionColor_; }
        [[nodiscard]] float GetWidth() const noexcept { return width_; }
        [[nodiscard]] float GetHeight() const noexcept { return height_; }
        [[nodiscard]] Render::Texture* GetTexture() const noexcept { return texture_; }
        [[nodiscard]] const Math::Rect& GetUVRect() const noexcept { return uvRect_; }
        [[nodiscard]] bool GetFlipX() const noexcept { return bFlipX_; }
        [[nodiscard]] bool GetFlipY() const noexcept { return bFlipY_; }
        [[nodiscard]] Math::Vector2<float> GetPivot() const noexcept { return pivot_; }
        [[nodiscard]] const String& GetMaterialId() const noexcept { return materialId_; }
        [[nodiscard]] SDL_BlendMode GetBlendMode() const noexcept { return blendMode_; }

        [[nodiscard]] String GetTypeName() const override { return "SpriteComponent"; }

    private:
        BPROPERTY()
        SDL_Color color_;

        BPROPERTY()
        SDL_Color tint_{255, 255, 255, 255};

        BPROPERTY()
        SDL_Color additiveTint_{0, 0, 0, 0};

        BPROPERTY()
        SDL_Color emissionColor_{0, 0, 0, 255};

        BPROPERTY()
        float width_;

        BPROPERTY()
        float height_;

        Render::Texture* texture_{nullptr};
        Math::Rect uvRect_{};
        bool hasCustomUV_{false};
        bool bFlipX_{false};
        bool bFlipY_{false};
        Math::Vector2<float> pivot_{0.5f, 0.5f};
        SDL_BlendMode blendMode_{SDL_BLENDMODE_BLEND};
        String materialId_{};
    };
}

#pragma once
#include "Components/Component.h"
#include "BixReflection.h"
#include "SDL3/SDL.h"
#include "Math/Rect.h"
#include "Math/Vector2.h"
#include "Ressources/Texture.h"
#include "Render/SpriteFrame.h"
#include "SpriteComponent.generated.h"


namespace BixEngine::Game
{
    BCLASS()

    class SpriteComponent : public Component
    {
        GENERATED_BODY()
        
        friend class SpriteAnimatorComponent;

    public:

        explicit SpriteComponent(Actor* owner);

        SpriteComponent(Actor* owner, SDL_Color color, float w, float h) : Component(owner), color_(color), width_(w), height_(h) {}

        void BeginPlay() override;

        void Render(Graphics::Renderer& renderer) const override;

        void ApplyFrame(const resources::SpriteFrame* frame, SDL_Color baseTint, float alpha);

        void SetColor(SDL_Color color) noexcept { color_ = color; }

        void SetTint(SDL_Color tint) noexcept { tint_ = tint; }

        void SetAdditiveTint(SDL_Color tint) noexcept { additiveTint_ = tint; }


        void SetDimensions(float w, float h) noexcept
        {
            width_ = w;
            height_ = h;
        }

        void SetTexture(resources::Texture* texture) noexcept;

        void SetUVRect(const Math::Rect& uvRect) noexcept;

        void SetFlipX(bool enabled) noexcept { bFlipX_ = enabled; }

        void SetFlipY(bool enabled) noexcept { bFlipY_ = enabled; }

        void SetPivot(const Math::Vector2<float>& pivot) noexcept { pivot_ = pivot; }

        void SetMaterialId(String materialId) noexcept { materialId_ = std::move(materialId); }

        void SetBlendMode(SDL_BlendMode mode) noexcept { blendMode_ = mode; }

        [[nodiscard]] SDL_Color GetColor() const noexcept { return color_; }
        [[nodiscard]] SDL_Color GetTint() const noexcept { return tint_; }
        [[nodiscard]] SDL_Color GetAdditiveTint() const noexcept { return additiveTint_; }

        [[nodiscard]] float GetWidth() const noexcept { return width_; }
        [[nodiscard]] float GetHeight() const noexcept { return height_; }

        [[nodiscard]] resources::Texture* GetTexture() const noexcept { return texture_; }
        [[nodiscard]] const Math::Rect& GetUVRect() const noexcept { return uvRect_; }

        [[nodiscard]] bool GetFlipX() const noexcept { return bFlipX_; }
        [[nodiscard]] bool GetFlipY() const noexcept { return bFlipY_; }

        [[nodiscard]] Math::Vector2<float> GetPivot() const noexcept { return pivot_; }

        [[nodiscard]] const String& GetMaterialId() const noexcept { return materialId_; }
        [[nodiscard]] SDL_BlendMode GetBlendMode() const noexcept { return blendMode_; }

        [[nodiscard]] String GetTypeName() const override { return "SpriteComponent"; }

    private:
        BPROPERTY()
        SDL_Color color_{255, 255, 255, 255};

        BPROPERTY()
        SDL_Color tint_{255, 255, 255, 255};

        BPROPERTY()
        SDL_Color additiveTint_{0, 0, 0, 0};

        BPROPERTY()
        float width_ = 32.f;

        BPROPERTY()
        float height_ = 32.f;

        BPROPERTY()
        resources::Texture* texture_{nullptr};

        Math::Rect uvRect_{};

        bool hasCustomUV_{false};

        bool bFlipX_{false};

        bool bFlipY_{false};

        Math::Vector2<float> pivot_{0.5f, 0.5f};

        SDL_BlendMode blendMode_{SDL_BLENDMODE_BLEND};

        String materialId_{};
    };
}

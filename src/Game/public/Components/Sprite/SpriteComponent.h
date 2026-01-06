#pragma once
#include "Components/Core/Component.h"
#include "Core/BixReflection.h"
#include "SDL3/SDL.h"
#include "Math/Rect.h"
#include "Math/Vector2.h"
#include "SpriteComponent.generated.h"
#include "Render/Sprite/SpriteFrame.h"


namespace BixEngine::Game
{
    BCLASS()
    class SpriteComponent : public Component
    {
        GENERATED_BODY()
        
        friend class SpriteAnimatorComponent;

    public:
        SpriteComponent() = default; 
        explicit SpriteComponent(Actor* owner);
        
        SpriteComponent(Actor* owner, SDL_Color color, Math::Vector2 textureScale)
            : Component(owner), color_(color), size_(textureScale)
        {}

        void BeginPlay() override;  
        void Update(float deltaTime) override;
        void Render(Graphics::Renderer& renderer) const override;

        // --- Animation & Apparence ---

        void ApplyFrame(const Resources::SpriteFrame* frame, SDL_Color baseTint, float alpha);

        // --- Setters ---

        void SetColor(SDL_Color color) noexcept { color_ = color; }
        void SetTint(SDL_Color tint) noexcept { tint_ = tint; }
        void SetAdditiveTint(SDL_Color tint) noexcept { additiveTint_ = tint; }

        void SetDimensions(float w, float h) noexcept { size_ = Math::Vector2(w, h); }

        void SetTexture(Resources::Texture* texture, bool resetUVs = true) noexcept;

        void SetUVRect(const Math::Rect& uvRect) noexcept;

        void SetFlipX(bool enabled) noexcept { bFlipX_ = enabled; }
        void SetFlipY(bool enabled) noexcept { bFlipY_ = enabled; }

        void SetPivot(const Math::Vector2& pivot) noexcept { pivot_ = pivot; }
        void SetMaterialId(String materialId) noexcept { materialId_ = std::move(materialId); }
        void SetBlendMode(SDL_BlendMode mode) noexcept { blendMode_ = mode; }

        // --- Getters ---

        [[nodiscard]] SDL_Color GetColor() const noexcept { return color_; }
        [[nodiscard]] SDL_Color GetTint() const noexcept { return tint_; }
        [[nodiscard]] SDL_Color GetAdditiveTint() const noexcept { return additiveTint_; }

        [[nodiscard]] float GetWidth() const noexcept { return size_.x; }
        [[nodiscard]] float GetHeight() const noexcept { return size_.y; }

        [[nodiscard]] Resources::Texture* GetTexture() const noexcept { return texture_; }
        [[nodiscard]] const Math::Rect& GetUVRect() const noexcept { return uvRect_; }

        [[nodiscard]] bool GetFlipX() const noexcept { return bFlipX_; }
        [[nodiscard]] bool GetFlipY() const noexcept { return bFlipY_; }

        [[nodiscard]] Math::Vector2 GetPivot() const noexcept { return pivot_; }
        [[nodiscard]] const String& GetMaterialId() const noexcept { return materialId_; }
        [[nodiscard]] SDL_BlendMode GetBlendMode() const noexcept { return blendMode_; }

        [[nodiscard]] String GetTypeName() const override { return "SpriteComponent"; }

    private:
        BPROPERTY(EditAnywhere, Category = "Rendering")
        SDL_Color color_{255, 255, 255, 255};

        BPROPERTY(EditAnywhere, Category = "Rendering")
        SDL_Color tint_{255, 255, 255, 255};

        SDL_Color additiveTint_{0, 0, 0, 0};

        BPROPERTY(EditAnywhere, Category = "Dimensions")
        Math::Vector2 size_ = Math::Vector2(150.f, 150.f);

        BPROPERTY(EditAnywhere, Category = "Rendering")
        Resources::Texture* texture_{nullptr};

        BPROPERTY(Category = "Internal")
        Math::Rect uvRect_{};

        BPROPERTY()
        bool hasCustomUV_{false};

        BPROPERTY(EditAnywhere, Category = "Transform")
        bool bFlipX_{false};

        BPROPERTY(EditAnywhere, Category = "Transform")
        bool bFlipY_{false};

        BPROPERTY(EditAnywhere, Category = "Transform")
        Math::Vector2 pivot_{0.5f, 0.5f};

        SDL_BlendMode blendMode_{SDL_BLENDMODE_BLEND};
        String materialId_{};
    };
}
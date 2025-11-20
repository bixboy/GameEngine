#include "Components/SpriteComponent.h"
#include "Actor.h"
#include "Renderer.h"
#include "Ressources/Texture.h"
#include "SDL3/SDL_render.h"
#include <algorithm>

namespace BixEngine::Game
{
    namespace
    {
        constexpr SDL_Color kDefaultSpriteColor{255, 255, 255, 255};
        constexpr float kDefaultSpriteWidth = 32.0f;
        constexpr float kDefaultSpriteHeight = 32.0f;

        [[nodiscard]] SDL_Color CombineColor(SDL_Color base, SDL_Color tint, SDL_Color additive) noexcept
        {
            auto mult = [](uint8_t a, uint8_t b) -> uint8_t
            {
                return static_cast<uint8_t>((static_cast<int>(a) * static_cast<int>(b)) / 255);
            };

            auto clamp8 = [](int value) -> uint8_t
            {
                return static_cast<uint8_t>(std::clamp(value, 0, 255));
            };

            SDL_Color result;
            result.r = clamp8(mult(base.r, tint.r) + additive.r);
            result.g = clamp8(mult(base.g, tint.g) + additive.g);
            result.b = clamp8(mult(base.b, tint.b) + additive.b);
            result.a = mult(base.a, tint.a);

            return result;
        }

        [[nodiscard]] SDL_FlipMode BuildFlipMode(bool flipX, bool flipY) noexcept
        {
            SDL_FlipMode mode = SDL_FLIP_NONE;
            if (flipX)
                mode = static_cast<SDL_FlipMode>(mode | SDL_FLIP_HORIZONTAL);

            if (flipY)
                mode = static_cast<SDL_FlipMode>(mode | SDL_FLIP_VERTICAL);

            return mode;
        }
    }


    
    SpriteComponent::SpriteComponent(Actor* owner) : Component(owner), color_(kDefaultSpriteColor), width_(kDefaultSpriteWidth), height_(kDefaultSpriteHeight), uvRect_(0.0f, 0.0f, kDefaultSpriteWidth, kDefaultSpriteHeight)
    {
    }

    void SpriteComponent::BeginPlay()
    {
        Component::BeginPlay();
    }

    void SpriteComponent::Render(Graphics::Renderer& renderer) const
    {
        auto pos = owner_->GetPosition();

        SDL_FRect destRect{pos.x - pivot_.x * width_, pos.y - pivot_.y * height_, width_, height_};

        if (texture_ && texture_->GetNativeHandle())
        {
            auto sdlTexture = static_cast<SDL_Texture*>(texture_->GetNativeHandle());
            SDL_SetTextureBlendMode(sdlTexture, blendMode_);

            const SDL_Color combined = CombineColor(color_, tint_, additiveTint_);
            SDL_SetTextureColorMod(sdlTexture, combined.r, combined.g, combined.b);
            SDL_SetTextureAlphaMod(sdlTexture, combined.a);

            SDL_FRect srcRect{uvRect_.X, uvRect_.Y, uvRect_.Width, uvRect_.Height};
            SDL_FPoint center{pivot_.x * width_, pivot_.y * height_};
            SDL_RenderTextureRotated(renderer.GetSDLRenderer(), sdlTexture, &srcRect, &destRect, 0.0, &center, BuildFlipMode(bFlipX_, bFlipY_));
        }
        else
        {
            const SDL_Color combined = CombineColor(color_, tint_, additiveTint_);
            renderer.SetColor(combined.r, combined.g, combined.b, combined.a);
            SDL_RenderFillRect(renderer.GetSDLRenderer(), &destRect);
        }
    }

    void SpriteComponent::ApplyFrame(const resources::SpriteFrame* frame, SDL_Color baseTint, float alpha)
    {
        if (!frame || !frame->IsValid())
        {
            SetTexture(nullptr);
            SetTint(baseTint);
            return;
        }

        SetTexture(frame->GetTexture());
        SetUVRect(frame->GetUVRect());

        SDL_Color tint = baseTint;
        tint.a = static_cast<uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * static_cast<float>(baseTint.a));
        SetTint(tint);
    }

    void SpriteComponent::SetTexture(resources::Texture* texture) noexcept
    {
        texture_ = texture;

        if (texture_ && !hasCustomUV_)
        {
            uvRect_ = {0.0f, 0.0f, static_cast<float>(texture_->GetWidth()), static_cast<float>(texture_->GetHeight())};
        }
        else if (!texture_)
        {
            hasCustomUV_ = false;
            uvRect_ = {0.0f, 0.0f, width_, height_};
        }
    }

    void SpriteComponent::SetUVRect(const Math::Rect& uvRect) noexcept
    {
        uvRect_ = uvRect;
        hasCustomUV_ = true;
    }
}

#include "Components/Sprite/SpriteComponent.h"
#include "Framework/Actor.h"
#include "Renderer.h"
#include "Ressources/RessourcesClass/Texture.h"
#include "Components/Core/CameraComponent.h"
#include "SDL3/SDL_render.h"
#include <algorithm>


namespace BixEngine::Game
{
    namespace
    {
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
    }
    
    SpriteComponent::SpriteComponent(Actor* owner) : Component(owner), uvRect_(0.0f, 0.0f, size_.x, size_.y)
    {
    }

    void SpriteComponent::BeginPlay()
    {
        Component::BeginPlay();
        
        if (texture_ && !hasCustomUV_)
        {
            uvRect_ = {0.0f, 0.0f, texture_->GetWidth(), texture_->GetHeight()};
        }
    }

    void SpriteComponent::Update(float deltaTime)
    {
        Component::Update(deltaTime);
    }

    void SpriteComponent::SetTexture(Resources::Texture* texture, bool resetUVs) noexcept
    {
        if (texture_ == texture)
            return;

        texture_ = texture;

        if (resetUVs || !hasCustomUV_)
        {
            if (texture_)
            {
                uvRect_ = {0.0f, 0.0f, texture_->GetWidth(), texture_->GetHeight()};
                hasCustomUV_ = false;
            }
            else
            {
                hasCustomUV_ = false;
                uvRect_ = {0.0f, 0.0f, size_.x, size_.y};
            }
        }
    }

    void SpriteComponent::SetUVRect(const Math::Rect& uvRect) noexcept
    {
        uvRect_ = uvRect;
        hasCustomUV_ = true;
    }

    void SpriteComponent::ApplyFrame(const Resources::SpriteFrame* frame, SDL_Color baseTint, float alpha)
    {
        if (!frame || !frame->IsValid())
        {
            SetTexture(nullptr);
            SetTint(baseTint);
            return;
        }

        SetTexture(frame->GetTexture(), false);
        SetUVRect(frame->GetUVRect());

        SDL_Color tint = baseTint;
        tint.a = static_cast<uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * static_cast<float>(baseTint.a));
        SetTint(tint);
    }

    void SpriteComponent::Render(Graphics::Renderer& renderer) const
    {
        if (!owner_)
            return;

        const float w = size_.x;
        const float h = size_.y;
        const float px = pivot_.x * w;
        const float py = pivot_.y * h;

        Math::Vector2 localTL = { -px, -py };
        Math::Vector2 localTR = { w - px, -py };
        Math::Vector2 localBL = { -px, h - py };
        Math::Vector2 localBR = { w - px, h - py };

        const Math::Transform& transform = owner_->GetTransform();
        Math::Matrix3 worldMatrix = transform.ToMatrix3();

        auto TransformPoint = [&](const Math::Vector2& p) -> Math::Vector2
        {
            Math::Vector3 input{ p.x, p.y, 1.0f };
            Math::Vector3 res = worldMatrix * input;
            return { res.x, res.y };
        };

        Math::Vector2 finalTL = TransformPoint(localTL);
        Math::Vector2 finalTR = TransformPoint(localTR);
        Math::Vector2 finalBL = TransformPoint(localBL);
        Math::Vector2 finalBR = TransformPoint(localBR);

        if (auto* cam = CameraComponent::GetMainCamera())
        {
            auto ToVec3 = [](const Math::Vector2& v)
            {
                return Math::Vector3(v.x, v.y, 0.0f);
            };
            
            finalTL = cam->WorldToScreen(ToVec3(finalTL));
            finalTR = cam->WorldToScreen(ToVec3(finalTR));
            finalBL = cam->WorldToScreen(ToVec3(finalBL));
            finalBR = cam->WorldToScreen(ToVec3(finalBR));
        }

        SDL_Texture* nativeTexture = texture_ ? texture_->GetNativeHandle() : nullptr;
        SDL_Color combined = CombineColor(color_, tint_, additiveTint_);
        SDL_FColor col = {
            static_cast<float>(combined.r) / 255.0f,
            static_cast<float>(combined.g) / 255.0f,
            static_cast<float>(combined.b) / 255.0f,
            static_cast<float>(combined.a) / 255.0f
        };

        Math::Vector2 uvs[4];
        if (nativeTexture)
        {
            float tw = texture_->GetWidth();
            float th = texture_->GetHeight();
            Math::Rect effectiveUV = uvRect_;

            if (effectiveUV.width <= 0.0f || effectiveUV.height <= 0.0f) {
                effectiveUV = {0.0f, 0.0f, tw, th};
            }

            if (tw > 0 && th > 0)
            {
                float invW = 1.0f / tw;
                float invH = 1.0f / th;
                
                float u0 = effectiveUV.x * invW;
                float v0 = effectiveUV.y * invH;
                float u1 = (effectiveUV.x + effectiveUV.width) * invW;
                float v1 = (effectiveUV.y + effectiveUV.height) * invH;

                if (bFlipX_) std::swap(u0, u1);
                if (bFlipY_) std::swap(v0, v1);

                uvs[0] = { u0, v0 }; // TL
                uvs[1] = { u1, v0 }; // TR
                uvs[2] = { u0, v1 }; // BL
                uvs[3] = { u1, v1 }; // BR
            }
            else {
                uvs[0] = {0,0}; uvs[1] = {1,0}; uvs[2] = {0,1}; uvs[3] = {1,1};
            }
            SDL_SetTextureBlendMode(nativeTexture, blendMode_);
        }
        else {
            uvs[0] = {0,0}; uvs[1] = {1,0}; uvs[2] = {0,1}; uvs[3] = {1,1};
        }
        
        
        SDL_Vertex vertices[4];
        vertices[0] = { {finalTL.x, finalTL.y}, col, {uvs[0].x, uvs[0].y} }; // Top-Left
        vertices[1] = { {finalTR.x, finalTR.y}, col, {uvs[1].x, uvs[1].y} }; // Top-Right
        vertices[2] = { {finalBR.x, finalBR.y}, col, {uvs[3].x, uvs[3].y} }; // Bottom-Right
        vertices[3] = { {finalBL.x, finalBL.y}, col, {uvs[2].x, uvs[2].y} }; // Bottom-Left

        int indices[] = { 0, 1, 2, 0, 2, 3 };

        SDL_RenderGeometry(renderer.GetSDLRenderer(), nativeTexture, vertices, 4, indices, 6);
    }
}
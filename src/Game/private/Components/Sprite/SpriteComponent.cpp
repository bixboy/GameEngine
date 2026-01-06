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
    
    
    SpriteComponent::SpriteComponent(Actor* owner) : Component(owner), uvRect_(0.0f, 0.0f, size_.x, size_.y)
    {
    }


    void SpriteComponent::BeginPlay()
    {
        Component::BeginPlay();
        lastTexture_ = texture_;
    }

    void SpriteComponent::Render(Graphics::Renderer& renderer) const
    {
        
        const Math::Transform& transform = owner_->GetTransform(); 
        Math::Matrix3 worldMatrix = transform.ToMatrix3(); 

        
        float w = size_.x;
        float h = size_.y;
        float px = pivot_.x * w;
        float py = pivot_.y * h;

        Math::Vector2<float> localTL = { -px, -py };
        Math::Vector2<float> localTR = { w - px, -py };
        Math::Vector2<float> localBL = { -px, h - py };
        Math::Vector2<float> localBR = { w - px, h - py };

        auto TransformPoint = [&](const Math::Vector2<float>& p) -> Math::Vector2<float>
        {
            Math::Vector3 input{ p.x, p.y, 1.0f };
            Math::Vector3 res = worldMatrix * input;
            return { res.x, res.y };
        };

        Math::Vector2<float> finalTL = TransformPoint(localTL);
        Math::Vector2<float> finalTR = TransformPoint(localTR);
        Math::Vector2<float> finalBL = TransformPoint(localBL);
        Math::Vector2<float> finalBR = TransformPoint(localBR);

        
        
        
        if (auto* cam = CameraComponent::GetMainCamera())
        {
            auto ToVec3 = [](const Math::Vector2<float>& v) { return Math::Vector3(v.x, v.y, 0.0f); };
            
            finalTL = cam->WorldToScreen(ToVec3(finalTL));
            finalTR = cam->WorldToScreen(ToVec3(finalTR));
            finalBL = cam->WorldToScreen(ToVec3(finalBL));
            finalBR = cam->WorldToScreen(ToVec3(finalBR));
        }

        
        SDL_Texture* nativeTexture = texture_ ? static_cast<SDL_Texture*>(texture_->GetNativeHandle()) : nullptr;
        
        SDL_Color combined = CombineColor(color_, tint_, additiveTint_);
        SDL_FColor col = { combined.r / 255.0f, combined.g / 255.0f, combined.b / 255.0f, combined.a / 255.0f };

        
        Math::Vector2<float> uvs[4];
        if (nativeTexture)
        {
            float tw = static_cast<float>(texture_->GetWidth());
            float th = static_cast<float>(texture_->GetHeight());

            Math::Rect effectiveUV = uvRect_;
            
            if (effectiveUV.width <= 0.0f || effectiveUV.height <= 0.0f)
            {
                effectiveUV = {0.0f, 0.0f, tw, th};
            }

            if (tw > 0 && th > 0)
            {
                float invW = 1.0f / tw;
                float invH = 1.0f / th;
                uvs[0] = { effectiveUV.x * invW, effectiveUV.y * invH }; 
                uvs[1] = { (effectiveUV.x + effectiveUV.width) * invW, effectiveUV.y * invH }; 
                uvs[2] = { effectiveUV.x * invW, (effectiveUV.y + effectiveUV.height) * invH }; 
                uvs[3] = { (effectiveUV.x + effectiveUV.width) * invW, (effectiveUV.y + effectiveUV.height) * invH }; 
            }
            else
            {
                uvs[0] = {0,0}; uvs[1] = {1,0}; uvs[2] = {0,1}; uvs[3] = {1,1};
            }
            
            SDL_SetTextureBlendMode(nativeTexture, blendMode_);
        }
        else
        {
            uvs[0] = {0,0}; uvs[1] = {1,0}; uvs[2] = {0,1}; uvs[3] = {1,1};
        }

        
        SDL_Vertex vertices[4];
        
        
        
        
        
        
        
        vertices[0] = { {finalTL.x, finalTL.y}, col, {uvs[0].x, uvs[0].y} };
        vertices[1] = { {finalTR.x, finalTR.y}, col, {uvs[1].x, uvs[1].y} }; 
        
        
        
        
        
        
        
        

        
        vertices[2] = { {finalBR.x, finalBR.y}, col, {uvs[3].x, uvs[3].y} };
        
        
        vertices[3] = { {finalBL.x, finalBL.y}, col, {uvs[2].x, uvs[2].y} };

        
        int indices[] = { 0, 1, 2, 0, 2, 3 };

        SDL_RenderGeometry(renderer.GetSDLRenderer(), nativeTexture, vertices, 4, indices, 6);
    }

    void SpriteComponent::ApplyFrame(const Resources::SpriteFrame* frame, SDL_Color baseTint, float alpha)
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

    void SpriteComponent::Update(float deltaTime)
    {
        Component::Update(deltaTime);
        
        if (texture_ != lastTexture_)
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
            
            lastTexture_ = texture_;
        }
    }



    void SpriteComponent::SetTexture(Resources::Texture* texture) noexcept
    {
        texture_ = texture;
        lastTexture_ = texture;

        if (texture_ && !hasCustomUV_)
        {
            uvRect_ = {0.0f, 0.0f, texture_->GetWidth(), texture_->GetHeight()};
        }
        else if (!texture_)
        {
            hasCustomUV_ = false;
            uvRect_ = {0.0f, 0.0f, size_.x, size_.y};
        }
    }

    void SpriteComponent::SetUVRect(const Math::Rect& uvRect) noexcept
    {
        uvRect_ = uvRect;
        hasCustomUV_ = true;
    }
}

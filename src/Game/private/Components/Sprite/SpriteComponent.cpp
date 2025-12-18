#include "Components/Sprite/SpriteComponent.h"
#include "Framework/Actor.h"
#include "Renderer.h"
#include "Ressources/RessourcesClass/Texture.h"
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
    }

    void SpriteComponent::Render(Graphics::Renderer& renderer) const
    {
        // Use the Actor's World Transform Matrix
        // This handles Position, Rotation (Yaw), Scale, and Parent Hierarchy automatically.
        const Math::Transform& transform = owner_->GetTransform(); 
        // Note: GetTransform returns by value in Object.h? Check. 
        // If it returns by value, we are fine, copy has 'parent' ptr.
        // Actually access to ToMatrix3() is what we need.
        Math::Matrix3 worldMatrix = transform.ToMatrix3(); 

        // 1. Calculate Local Vertices (Object Space) based on Size and Pivot
        // The actor's transform applies TO these points.
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
            // Vector3 input(p.x, p.y, 1.0f);
            // Vector3 res = worldMatrix * input;
            // But Transform::TransformPoint does exactly this logic if we had the instance.
            // Or we do it manually with matrix:
            Math::Vector3 input{ p.x, p.y, 1.0f };
            Math::Vector3 res = worldMatrix * input;
            return { res.x, res.y };
        };

        Math::Vector2<float> worldTL = TransformPoint(localTL);
        Math::Vector2<float> worldTR = TransformPoint(localTR);
        Math::Vector2<float> worldBL = TransformPoint(localBL);
        Math::Vector2<float> worldBR = TransformPoint(localBR);

        // 2. Prepare Colors
        SDL_Texture* nativeTexture = texture_ ? static_cast<SDL_Texture*>(texture_->GetNativeHandle()) : nullptr;
        
        SDL_Color combined = CombineColor(color_, tint_, additiveTint_);
        SDL_FColor col = { combined.r / 255.0f, combined.g / 255.0f, combined.b / 255.0f, combined.a / 255.0f };

        // 3. Prepare UVs
        Math::Vector2<float> uvs[4];
        if (nativeTexture)
        {
            // Normalize UVs
            float tw = static_cast<float>(texture_->GetWidth());
            float th = static_cast<float>(texture_->GetHeight());
            if (tw > 0 && th > 0)
            {
                float invW = 1.0f / tw;
                float invH = 1.0f / th;
                uvs[0] = { uvRect_.X * invW, uvRect_.Y * invH }; // TL
                uvs[1] = { (uvRect_.X + uvRect_.Width) * invW, uvRect_.Y * invH }; // TR
                uvs[2] = { uvRect_.X * invW, (uvRect_.Y + uvRect_.Height) * invH }; // BL
                uvs[3] = { (uvRect_.X + uvRect_.Width) * invW, (uvRect_.Y + uvRect_.Height) * invH }; // BR
            }
            
            SDL_SetTextureBlendMode(nativeTexture, blendMode_);
            // SDL_RenderGeometry uses the colors passed in vertices, checks texture alpha logic.
            // But SDL_RenderGeometry respects SetTextureColorMod? Docs say:
            // "The color of the vertices is multiplied with the texture color."
            // So we can burn color into vertices. 
        }
        else
        {
            uvs[0] = {0,0}; uvs[1] = {1,0}; uvs[2] = {0,1}; uvs[3] = {1,1};
        }

        // 4. Submit Geometry
        SDL_Vertex vertices[4];
        // Order: TL, TR, BR, BL - wait, Indices define the mesh.
        // Let's stick to standard TL, TR, BR, BL order generally used in quad logic or simpler:
        // 0: TL
        // 1: TR
        // 2: BR
        // 3: BL
        
        vertices[0] = { {worldTL.x, worldTL.y}, col, {uvs[0].x, uvs[0].y} };
        vertices[1] = { {worldTR.x, worldTR.y}, col, {uvs[1].x, uvs[1].y} }; // TR 
        
        // Wait, my uvs array filling was:
        // 2 was BL, 3 was BR in my loop above?
        // Let's match explicit indices.
        // uvs[0] -> TL
        // uvs[1] -> TR
        // uvs[2] -> BL
        // uvs[3] -> BR relative to Rect (X, Y)

        // Vertex 2: BR
        vertices[2] = { {worldBR.x, worldBR.y}, col, {uvs[3].x, uvs[3].y} };
        
        // Vertex 3: BL
        vertices[3] = { {worldBL.x, worldBL.y}, col, {uvs[2].x, uvs[2].y} };

        // Indices: TL(0) -> TR(1) -> BR(2); TL(0) -> BR(2) -> BL(3)
        int indices[] = { 0, 1, 2, 0, 2, 3 };

        SDL_RenderGeometry(renderer.GetSDLRenderer(), nativeTexture, vertices, 4, indices, 6);
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



    void SpriteComponent::SetTexture(resources::Texture* texture) noexcept
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

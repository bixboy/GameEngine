#include "Game/Components/SpriteAnimatorComponent.h"
#include "Game/Actor.h"
#include "Game/Components/SpriteComponent.h"
#include "Graphics/Renderer.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace BixEngine::Game
{
    namespace
    {
        [[nodiscard]] bool AreRectsEqual(const Math::Rect& lhs, const Math::Rect& rhs) noexcept
        {
            constexpr float kRectEpsilon = 0.0001f;
            return std::fabs(lhs.X - rhs.X) < kRectEpsilon &&
                   std::fabs(lhs.Y - rhs.Y) < kRectEpsilon &&
                   std::fabs(lhs.Width - rhs.Width) < kRectEpsilon &&
                   std::fabs(lhs.Height - rhs.Height) < kRectEpsilon;
        }
    }

    SpriteAnimatorComponent::SpriteAnimatorComponent(Actor* owner)
        : Component(owner)
    {
    }

    void SpriteAnimatorComponent::BeginPlay()
    {
        Component::BeginPlay();

        if (spriteBindings_.empty())
        {
            if (auto* sprite = owner_->GetComponent<SpriteComponent>())
            {
                AddSpriteLayer(sprite);
            }
        }

        EvaluateStateMachine();
    }

    void SpriteAnimatorComponent::Update(float deltaTime)
    {
        const float dt = deltaTimeOverride_.has_value() ? deltaTimeOverride_.value() : deltaTime;
        primaryAnimator_.Update(deltaTime, deltaTimeOverride_);
        if (bBlending_)
        {
            blendAnimator_.Update(deltaTime, deltaTimeOverride_);
            UpdateBlend(dt);
        }

        EvaluateStateMachine();

        if (!queuedState_.IsEmpty())
        {
            auto stateIt = std::find_if(states_.begin(), states_.end(), [&](const SpriteAnimationState& candidate)
            {
                return candidate.StateName == queuedState_;
            });

            if (stateIt != states_.end())
            {
                if (primaryAnimator_.HasAnimation(stateIt->AnimationName))
                {
                    if (!activeState_.IsEmpty())
                    {
                        const String previousAnimation = primaryAnimator_.GetCurrentAnimationName();
                        if (!previousAnimation.IsEmpty() && primaryAnimator_.HasAnimation(previousAnimation))
                        {
                            blendAnimator_.Play(previousAnimation);
                            blendAnimator_.Seek(primaryAnimator_.GetNormalizedTime());
                            bBlending_ = stateIt->BlendDuration > 0.0f;
                            blendDuration_ = stateIt->BlendDuration;
                            blendTimer_ = 0.0f;
                        }
                    }

                    primaryAnimator_.Play(stateIt->AnimationName);
                    activeState_ = stateIt->StateName;
                    queuedState_.Clear();

                    if (OnAnimationStart)
                        OnAnimationStart(stateIt->AnimationName);
                }
                else
                {
                    queuedState_.Clear();
                }
            }
        }

        if (bUsingTemporary_)
        {
            temporaryTimer_ -= dt;
            if (temporaryTimer_ <= 0.0f)
            {
                bUsingTemporary_ = false;
                if (!fallbackState_.IsEmpty())
                {
                    queuedState_ = fallbackState_;
                    fallbackState_.Clear();
                }
            }
        }

        const Render::SpriteFrame* primaryFrame = primaryAnimator_.GetCurrentFrame();
        const Render::SpriteFrame* blendFrame = bBlending_ ? blendAnimator_.GetCurrentFrame() : nullptr;
        const float blendWeight = bBlending_ && blendDuration_ > 0.0f ? std::clamp(blendTimer_ / blendDuration_, 0.0f, 1.0f) : 0.0f;

        if (!spriteBindings_.empty())
        {
            if (blendFrame && spriteBindings_.size() > 1)
            {
                ApplyFrame(spriteBindings_[0], primaryFrame, 1.0f - blendWeight);
                ApplyFrame(spriteBindings_[1], blendFrame, blendWeight);
                for (size_t i = 2; i < spriteBindings_.size(); ++i)
                {
                    ApplyFrame(spriteBindings_[i], primaryFrame, 1.0f);
                }
            }
            else
            {
                for (auto& binding : spriteBindings_)
                {
                    ApplyFrame(binding, primaryFrame, 1.0f);
                }
            }
        }

        if (OnFrameChanged && lastFrameIndex_ != primaryAnimator_.GetCurrentFrameIndex())
        {
            lastFrameIndex_ = primaryAnimator_.GetCurrentFrameIndex();
            OnFrameChanged(primaryAnimator_.GetCurrentAnimationName(), lastFrameIndex_);
        }

        if (OnAnimationEnd && wasPlaying_ && !primaryAnimator_.IsPlaying())
        {
            OnAnimationEnd(primaryAnimator_.GetCurrentAnimationName());
        }

        wasPlaying_ = primaryAnimator_.IsPlaying();

        if (bDebugMode_)
        {
            UpdateDebugWindow();
        }
    }

    void SpriteAnimatorComponent::AddAnimation(Render::SpriteAnimation animation)
    {
        primaryAnimator_.AddAnimation(animation);
        blendAnimator_.AddAnimation(animation);
    }

    void SpriteAnimatorComponent::Play(const String& name)
    {
        primaryAnimator_.Play(name);
        activeState_.Clear();
        queuedState_.Clear();
        bBlending_ = false;
        blendAnimator_.Stop();
        if (OnAnimationStart)
            OnAnimationStart(name);
    }

    void SpriteAnimatorComponent::PlayOnceThen(const String& name, const String& next)
    {
        primaryAnimator_.PlayOnceThen(name, next);
        activeState_.Clear();
        queuedState_ = next;
        bBlending_ = false;
        blendAnimator_.Stop();
        if (OnAnimationStart)
            OnAnimationStart(name);
    }

    void SpriteAnimatorComponent::PlayForDuration(const String& name, float seconds, const String& fallbackState)
    {
        primaryAnimator_.Play(name);
        bUsingTemporary_ = true;
        temporaryTimer_ = seconds;
        fallbackState_ = fallbackState;
        activeState_.Clear();
        bBlending_ = false;
        blendAnimator_.Stop();
        if (OnAnimationStart)
            OnAnimationStart(name);
    }

    void SpriteAnimatorComponent::Pause()
    {
        primaryAnimator_.Pause();
        blendAnimator_.Pause();
    }

    void SpriteAnimatorComponent::Stop()
    {
        primaryAnimator_.Stop();
        blendAnimator_.Stop();
        activeState_.Clear();
        queuedState_.Clear();
        bBlending_ = false;
        blendTimer_ = 0.0f;
    }

    void SpriteAnimatorComponent::SetPlaybackSpeed(float speed)
    {
        primaryAnimator_.SetSpeed(std::max(speed, 0.0f));
        blendAnimator_.SetSpeed(std::max(speed, 0.0f));
    }

    bool SpriteAnimatorComponent::IsPlaying() const noexcept
    {
        return primaryAnimator_.IsPlaying();
    }

    void SpriteAnimatorComponent::AddSpriteLayer(SpriteComponent* sprite)
    {
        if (!sprite)
            return;

        auto found = std::find_if(spriteBindings_.begin(), spriteBindings_.end(), [sprite](const SpriteBinding& binding)
        {
            return binding.Component == sprite;
        });

        if (found != spriteBindings_.end())
            return;

        SpriteBinding binding{};
        binding.Component = sprite;
        binding.BaseTint = sprite->GetTint();
        spriteBindings_.push_back(binding);
    }

    void SpriteAnimatorComponent::RemoveSpriteLayer(SpriteComponent* sprite)
    {
        spriteBindings_.erase(std::remove_if(spriteBindings_.begin(), spriteBindings_.end(), [sprite](const SpriteBinding& binding)
        {
            return binding.Component == sprite;
        }), spriteBindings_.end());
    }

    void SpriteAnimatorComponent::ClearSpriteLayers()
    {
        spriteBindings_.clear();
    }

    void SpriteAnimatorComponent::AddState(SpriteAnimationState state)
    {
        states_.push_back(std::move(state));
    }

    void SpriteAnimatorComponent::SetDefaultState(String stateName)
    {
        defaultState_ = std::move(stateName);
    }

    void SpriteAnimatorComponent::SetTargetSprite(SpriteComponent* sprite) noexcept
    {
        ClearSpriteLayers();
        AddSpriteLayer(sprite);
    }

    void SpriteAnimatorComponent::ApplyFrame(SpriteBinding& binding, const Render::SpriteFrame* frame, float alphaWeight)
    {
        if (!binding.Component)
            return;

        if (!frame || !frame->handle)
        {
            if (binding.CurrentTexture)
            {
                binding.CurrentTexture = nullptr;
                binding.Component->SetTexture(nullptr);
            }
            binding.Component->SetTint(binding.BaseTint);
            return;
        }

        Render::Texture* texture = frame->GetTexture();
        if (binding.CurrentTexture != texture)
        {
            binding.CurrentTexture = texture;
            binding.Component->SetTexture(texture);
        }

        const Math::Rect& uv = frame->GetUVRect();
        if (!AreRectsEqual(binding.CurrentUV, uv))
        {
            binding.CurrentUV = uv;
            binding.Component->SetUVRect(uv);
        }

        SDL_Color tint = binding.BaseTint;
        tint.a = static_cast<uint8_t>(std::clamp(alphaWeight, 0.0f, 1.0f) * static_cast<float>(binding.BaseTint.a));
        binding.Component->SetTint(tint);
    }

    void SpriteAnimatorComponent::EvaluateStateMachine()
    {
        if (bUsingTemporary_)
            return;

        for (const auto& state : states_)
        {
            if (state.Condition && state.Condition(*this))
            {
                if (activeState_ != state.StateName)
                {
                    queuedState_ = state.StateName;
                }
                return;
            }
        }

        if (!defaultState_.IsEmpty() && activeState_ != defaultState_)
        {
            queuedState_ = defaultState_;
        }
    }

    void SpriteAnimatorComponent::UpdateBlend(float deltaTime)
    {
        if (!bBlending_)
            return;

        blendTimer_ += deltaTime;
        if (blendTimer_ >= blendDuration_)
        {
            bBlending_ = false;
            blendTimer_ = blendDuration_;
            if (spriteBindings_.size() > 1)
            {
                // restore secondary tint
                spriteBindings_[1].Component->SetTint(spriteBindings_[1].BaseTint);
            }
        }
    }

    void SpriteAnimatorComponent::UpdateDebugWindow()
    {
        if (ImGui::Begin("Sprite Animator Debug"))
        {
            ImGui::Text("Active Animation: %s", primaryAnimator_.GetCurrentAnimationName().c_str());
            ImGui::Text("Normalized Time: %.2f", primaryAnimator_.GetNormalizedTime());
            ImGui::Text("Frame Index: %zu", primaryAnimator_.GetCurrentFrameIndex());
            ImGui::Text("Playing: %s", primaryAnimator_.IsPlaying() ? "Yes" : "No");
            ImGui::Text("State: %s", activeState_.c_str());
        }
        ImGui::End();

        if (const Render::SpriteFrame* frame = primaryAnimator_.GetCurrentFrame())
        {
            if (ImGui::Begin("Sprite Preview"))
            {
                if (frame->handle && frame->GetTexture())
                {
                    ImTextureID textureId = reinterpret_cast<ImTextureID>(frame->GetTexture()->GetNativeHandle());
                    const float texWidth = static_cast<float>(frame->GetTexture()->GetWidth());
                    const float texHeight = static_cast<float>(frame->GetTexture()->GetHeight());
                    const ImVec2 size(frame->GetUVRect().Width, frame->GetUVRect().Height);
                    const ImVec2 uv0(frame->GetUVRect().X / texWidth, frame->GetUVRect().Y / texHeight);
                    const ImVec2 uv1((frame->GetUVRect().X + frame->GetUVRect().Width) / texWidth,
                                     (frame->GetUVRect().Y + frame->GetUVRect().Height) / texHeight);
                    ImGui::Image(textureId, size, uv0, uv1);
                }
                else
                {
                    ImGui::TextUnformatted("No texture bound");
                }
            }
            ImGui::End();
        }
    }
}

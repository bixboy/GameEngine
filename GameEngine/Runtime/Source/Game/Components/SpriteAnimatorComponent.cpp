#include "Game/Components/SpriteAnimatorComponent.h"
#include "Game/Actor.h"
#include "Game/Components/SpriteComponent.h"
#include "Graphics/Renderer.h"
#include "Engine/Render/SpriteAtlasUtils.h"
#include "Engine/Render/TextureManager.h"
#include "Core/Logger.h"
#include "imgui.h"
#include <array>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

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

    namespace
    {
        bool EditStringField(const char* label, String& value, bool* committed = nullptr)
        {
            std::array<char, 512> buffer{};
            const std::string_view current = value.View();
            const std::size_t copyLength = std::min(buffer.size() - 1, current.size());
            std::memcpy(buffer.data(), current.data(), copyLength);
            buffer[copyLength] = '\0';
            const bool edited = ImGui::InputText(label, buffer.data(), buffer.size());
            const bool commit = ImGui::IsItemDeactivatedAfterEdit();

            if (edited)
            {
                value = buffer.data();
            }

            if (committed)
            {
                *committed = commit;
            }

            return edited;
        }

        bool EditIntegerField(const char* label, int& value, int minValue = 0, bool* committed = nullptr)
        {
            int temp = value;
            const bool edited = ImGui::InputInt(label, &temp);
            const bool commit = ImGui::IsItemDeactivatedAfterEdit();

            if (edited)
            {
                value = std::max(temp, minValue);
            }

            if (committed)
            {
                *committed = commit;
            }

            return edited;
        }

        bool EditFloatField(const char* label, float& value, float minValue = 0.0f, bool* committed = nullptr)
        {
            float temp = value;
            const bool edited = ImGui::InputFloat(label, &temp);
            const bool commit = ImGui::IsItemDeactivatedAfterEdit();

            if (edited)
            {
                value = std::max(temp, minValue);
            }

            if (committed)
            {
                *committed = commit;
            }

            return edited;
        }
    }

    SpriteAnimatorComponent::SpriteAnimatorComponent(Actor* owner): Component(owner)
    {
        clipConfigs_.push_back({});
        clipConfigs_.front().Name = "Animation";
        initialClipName_ = clipConfigs_.front().Name;
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

        if (clipConfigsDirty_)
        {
            ReloadAnimations();
        }
        else
        {
            TryAutoPlay();
        }

        EvaluateStateMachine();
    }

    void SpriteAnimatorComponent::Update(float deltaTime)
    {
        if (clipConfigsDirty_)
        {
            ReloadAnimations();
        }

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

    void SpriteAnimatorComponent::AddAnimation(const Render::SpriteAnimation& animation)
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

    void SpriteAnimatorComponent::SetSpriteSheet(SpriteSheetConfig sheetConfig)
    {
        sheetConfig_.TexturePath = std::move(sheetConfig.TexturePath);
        sheetConfig_.Columns = std::max(sheetConfig.Columns, 1);
        sheetConfig_.Rows = std::max(sheetConfig.Rows, 1);
        sheetConfig_.Padding = std::max(sheetConfig.Padding, 0);
        sheetConfig_.Margin = std::max(sheetConfig.Margin, 0);

        clipConfigsDirty_ = true;
        clipBuildFailed_ = false;
        ReloadAnimations();
    }

    void SpriteAnimatorComponent::SetClips(std::vector<SpriteAnimationClipConfig> clips)
    {
        clipConfigs_ = std::move(clips);
        EnsureDefaultClip();
        clipConfigsDirty_ = true;
        clipBuildFailed_ = false;
        ReloadAnimations();
    }

    void SpriteAnimatorComponent::SetInitialClip(String clipName) noexcept
    {
        initialClipName_ = std::move(clipName);
    }

    void SpriteAnimatorComponent::SetAutoPlay(bool enabled) noexcept
    {
        const bool wasDisabled = !bAutoPlayOnLoad_;
        bAutoPlayOnLoad_ = enabled;
        if (enabled && wasDisabled)
        {
            TryAutoPlay();
        }
    }

    void SpriteAnimatorComponent::ReloadAnimations()
    {
        Graphics::Renderer* renderer = Graphics::Renderer::Get();
        if (!renderer)
        {
            return;
        }

        SDL_Renderer* sdlRenderer = renderer->GetSDLRenderer();
        if (!sdlRenderer)
        {
            return;
        }

        const bool rebuilt = RebuildAnimations(sdlRenderer);
        clipBuildFailed_ = !rebuilt;
        clipConfigsDirty_ = false;

        if (rebuilt)
        {
            TryAutoPlay();
        }
    }

    void SpriteAnimatorComponent::DrawInspectorUI()
    {
        bool autoPlay = bAutoPlayOnLoad_;
        if (ImGui::Checkbox("Auto-play on load", &autoPlay))
        {
            SetAutoPlay(autoPlay);
        }

        String initialClipCopy = initialClipName_;
        bool initialClipCommitted = false;
        if (EditStringField("Initial clip", initialClipCopy, &initialClipCommitted))
        {
            SetInitialClip(initialClipCopy);
            if (initialClipCommitted && primaryAnimator_.HasAnimation(initialClipName_))
            {
                Play(initialClipName_);
            }
        }

        ImGui::Separator();

        bool sheetCommitted = false;
        if (EditStringField("Texture path", sheetConfig_.TexturePath, &sheetCommitted))
        {
            if (sheetCommitted)
            {
                clipBuildFailed_ = false;
                clipConfigsDirty_ = true;
                ReloadAnimations();
            }
        }

        int columns = sheetConfig_.Columns;
        bool columnsCommitted = false;
        if (EditIntegerField("Columns", columns, 1, &columnsCommitted))
        {
            sheetConfig_.Columns = columns;
            if (columnsCommitted)
            {
                clipBuildFailed_ = false;
                clipConfigsDirty_ = true;
                ReloadAnimations();
            }
        }

        int rows = sheetConfig_.Rows;
        bool rowsCommitted = false;
        if (EditIntegerField("Rows", rows, 1, &rowsCommitted))
        {
            sheetConfig_.Rows = rows;
            if (rowsCommitted)
            {
                clipBuildFailed_ = false;
                clipConfigsDirty_ = true;
                ReloadAnimations();
            }
        }

        int padding = sheetConfig_.Padding;
        bool paddingCommitted = false;
        if (EditIntegerField("Padding", padding, 0, &paddingCommitted))
        {
            sheetConfig_.Padding = padding;
            if (paddingCommitted)
            {
                clipBuildFailed_ = false;
                clipConfigsDirty_ = true;
                ReloadAnimations();
            }
        }

        int margin = sheetConfig_.Margin;
        bool marginCommitted = false;
        if (EditIntegerField("Margin", margin, 0, &marginCommitted))
        {
            sheetConfig_.Margin = margin;
            if (marginCommitted)
            {
                clipBuildFailed_ = false;
                clipConfigsDirty_ = true;
                ReloadAnimations();
            }
        }

        ImGui::Separator();

        int index = 0;
        int clipToRemove = -1;
        for (auto& clip : clipConfigs_)
        {
            ImGui::PushID(index);
            const std::string headerLabel = clip.Name.IsEmpty() ? ("Clip " + std::to_string(index + 1)) : clip.Name.Std();
            if (ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                const String previousName = clip.Name;
                bool nameCommitted = false;
                if (EditStringField("Name", clip.Name, &nameCommitted))
                {
                    if (nameCommitted)
                    {
                        if (initialClipName_ == previousName)
                        {
                            SetInitialClip(clip.Name);
                        }
                        clipBuildFailed_ = false;
                        clipConfigsDirty_ = true;
                        ReloadAnimations();
                    }
                }

                int startFrame = clip.StartFrame;
                bool startCommitted = false;
                if (EditIntegerField("Start frame", startFrame, 0, &startCommitted))
                {
                    clip.StartFrame = startFrame;
                    if (startCommitted)
                    {
                        clipBuildFailed_ = false;
                        clipConfigsDirty_ = true;
                        ReloadAnimations();
                    }
                }

                int frameCount = clip.FrameCount;
                bool countCommitted = false;
                if (EditIntegerField("Frame count", frameCount, 0, &countCommitted))
                {
                    clip.FrameCount = frameCount;
                    if (countCommitted)
                    {
                        clipBuildFailed_ = false;
                        clipConfigsDirty_ = true;
                        ReloadAnimations();
                    }
                }

                float frameRate = clip.FrameRate;
                bool rateCommitted = false;
                if (EditFloatField("Frame rate", frameRate, 1.0f, &rateCommitted))
                {
                    clip.FrameRate = frameRate;
                    if (rateCommitted)
                    {
                        clipBuildFailed_ = false;
                        clipConfigsDirty_ = true;
                        ReloadAnimations();
                    }
                }

                if (ImGui::Checkbox("Loop", &clip.bLoop))
                {
                    clipBuildFailed_ = false;
                    clipConfigsDirty_ = true;
                    ReloadAnimations();
                }

                if (ImGui::Button("Remove clip"))
                {
                    clipToRemove = index;
                }
            }
            ImGui::PopID();
            ++index;
        }

        if (clipToRemove >= 0 && clipToRemove < static_cast<int>(clipConfigs_.size()))
        {
            const String removedName = clipConfigs_[clipToRemove].Name;
            clipConfigs_.erase(clipConfigs_.begin() + clipToRemove);
            clipBuildFailed_ = false;
            clipConfigsDirty_ = true;
            if (initialClipName_ == removedName)
            {
                if (!clipConfigs_.empty())
                {
                    SetInitialClip(clipConfigs_.front().Name);
                }
                else
                {
                    initialClipName_.Clear();
                }
            }
            ReloadAnimations();
        }

        if (ImGui::Button("Add clip"))
        {
            SpriteAnimationClipConfig clip{};
            clip.Name = String{"Animation"} + String{std::to_string(static_cast<int>(clipConfigs_.size()) + 1)};
            clipConfigs_.push_back(std::move(clip));
            clipBuildFailed_ = false;
            clipConfigsDirty_ = true;
            ReloadAnimations();
        }

        if (clipBuildFailed_)
        {
            ImGui::TextWrapped("Unable to rebuild animations with the current settings. Check the sheet path and frame range.");
        }
    }

    bool SpriteAnimatorComponent::BuildAnimationFromClip(const SpriteAnimationClipConfig& clipConfig,
                                                        const std::vector<Render::SpriteFrame>& frames)
    {
        if (frames.empty())
        {
            return false;
        }

        const int totalFrames = static_cast<int>(frames.size());
        const int safeStart = std::clamp(clipConfig.StartFrame, 0, std::max(totalFrames - 1, 0));
        int frameCount = clipConfig.FrameCount > 0 ? clipConfig.FrameCount : (totalFrames - safeStart);
        frameCount = std::clamp(frameCount, 0, totalFrames - safeStart);
        if (frameCount == 0)
        {
            return false;
        }

        Render::SpriteAnimation animation{};
        animation.Name = clipConfig.Name;
        animation.FrameRate = std::max(clipConfig.FrameRate, 1.0f);
        animation.bLoop = clipConfig.bLoop;
        animation.Frames.reserve(static_cast<std::size_t>(frameCount));
        for (int i = 0; i < frameCount; ++i)
        {
            animation.Frames.push_back(frames[static_cast<std::size_t>(safeStart + i)]);
        }

        AddAnimation(animation);
        return true;
    }

    void SpriteAnimatorComponent::EnsureDefaultClip()
    {
        if (clipConfigs_.empty())
        {
            SpriteAnimationClipConfig clip{};
            clip.Name = "Animation";
            clipConfigs_.push_back(std::move(clip));
        }

        int clipIndex = 1;
        for (auto& clip : clipConfigs_)
        {
            if (clip.Name.IsEmpty())
            {
                clip.Name = String{"Animation"} + String{std::to_string(clipIndex)};
            }
            ++clipIndex;
        }

        if (initialClipName_.IsEmpty() && !clipConfigs_.empty())
        {
            initialClipName_ = clipConfigs_.front().Name;
        }
    }

    bool SpriteAnimatorComponent::RebuildAnimations(SDL_Renderer* sdlRenderer)
    {
        if (!sdlRenderer)
        {
            return false;
        }

        if (sheetConfig_.TexturePath.IsEmpty())
        {
            return false;
        }

        Render::TextureManager& textureManager = Render::TextureManager::Get();
        auto texture = textureManager.LoadTexture(sheetConfig_.TexturePath, sdlRenderer);
        if (!texture)
        {
            LOG_ERROR(String{"[SpriteAnimatorComponent] Failed to load texture: "} + sheetConfig_.TexturePath);
            return false;
        }

        const int columns = std::max(sheetConfig_.Columns, 1);
        const int rows = std::max(sheetConfig_.Rows, 1);
        const int padding = std::max(sheetConfig_.Padding, 0);
        const int margin = std::max(sheetConfig_.Margin, 0);

        std::vector<Render::SpriteFrame> frames = Render::SpriteAtlasUtils::LoadFramesFromAtlas(*texture, columns, rows, padding, margin);
        if (frames.empty())
        {
            LOG_ERROR(String{"[SpriteAnimatorComponent] No frames generated for sheet: "} + sheetConfig_.TexturePath);
            return false;
        }

        EnsureDefaultClip();

        primaryAnimator_ = Render::SpriteAnimator();
        blendAnimator_ = Render::SpriteAnimator();
        activeState_.Clear();
        queuedState_.Clear();
        bBlending_ = false;
        blendTimer_ = 0.0f;
        blendDuration_ = 0.0f;
        lastFrameIndex_ = std::numeric_limits<size_t>::max();
        wasPlaying_ = false;

        bool anyBuilt = false;
        for (const auto& clip : clipConfigs_)
        {
            if (clip.Name.IsEmpty())
            {
                continue;
            }

            const bool built = BuildAnimationFromClip(clip, frames);
            if (built)
            {
                anyBuilt = true;
            }
        }

        if (anyBuilt)
        {
            if (initialClipName_.IsEmpty() || !primaryAnimator_.HasAnimation(initialClipName_))
            {
                initialClipName_ = clipConfigs_.front().Name;
            }
        }

        return anyBuilt;
    }

    void SpriteAnimatorComponent::ApplyFrame(SpriteBinding& binding, const Render::SpriteFrame* frame, float alphaWeight)
    {
        if (!binding.Component)
            return;

        binding.Component->ApplyFrame(frame, binding.BaseTint, alphaWeight);
    }

    void SpriteAnimatorComponent::TryAutoPlay()
    {
        if (!bAutoPlayOnLoad_)
        {
            return;
        }

        String clipToPlay = initialClipName_;
        if (clipToPlay.IsEmpty())
        {
            for (const auto& clip : clipConfigs_)
            {
                if (!clip.Name.IsEmpty())
                {
                    clipToPlay = clip.Name;
                    break;
                }
            }
        }

        if (clipToPlay.IsEmpty())
        {
            return;
        }

        if (!primaryAnimator_.HasAnimation(clipToPlay))
        {
            return;
        }

        Play(clipToPlay);
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

#pragma once
#include <SDL3/SDL_stdinc.h>


namespace BixEngine::Core
{
    
    class Timer
    {
    public:
        
        void Tick();


        
        
        void SleepUntilNextFrame(float targetDelta);


        
        void SetTimeScale(float scale) noexcept { timeScale_ = scale; }


        
        [[nodiscard]] float GetDeltaTime() const noexcept { return deltaTime_; }

        
        [[nodiscard]] float GetScaledDeltaTime() const noexcept { return deltaTime_ * timeScale_; }

        
        [[nodiscard]] float GetFPS() const noexcept { return deltaTime_ > 0.0f ? 1.0f / deltaTime_ : 0.0f; }

        
        [[nodiscard]] float GetTotalTime() const noexcept { return totalTime_; }

        
        [[nodiscard]] float GetTimeScale() const noexcept { return timeScale_; }

    private:
        Uint64 lastCounter_{0};
        float deltaTime_{0.0f};
        float totalTime_{0.0f};
        float timeScale_{1.0f};
    };
}

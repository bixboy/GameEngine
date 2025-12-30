#include "Time/Timer.h"
#include <SDL3/SDL_timer.h>
#include <algorithm>

namespace BixEngine::Core
{
    Timer::Timer()
    {
        frequency_ = SDL_GetPerformanceFrequency();
        Reset();
    }

    void Timer::Reset()
    {
        lastCounter_ = SDL_GetPerformanceCounter();
        deltaTime_ = 0.0f;
        totalTime_ = 0.0f;
        realTimeSinceStartup_ = 0.0f;
        fps_ = 0.0f;
        frameCount_ = 0;
        fpsTimer_ = 0.0f;
    }

    void Timer::SleepUntilNextFrame(float targetDeltaTime)
    {
        const Uint64 frequency = SDL_GetPerformanceFrequency();
        const Uint64 targetTicks = static_cast<Uint64>(targetDeltaTime * static_cast<double>(frequency));
    
        Uint64 currentCounter = SDL_GetPerformanceCounter();
        Uint64 elapsedTicks = currentCounter - lastCounter_;

        if (elapsedTicks < targetTicks)
        {
            const Uint64 remainingTicks = targetTicks - elapsedTicks;
            const double remainingMs = (static_cast<double>(remainingTicks) * 1000.0) / static_cast<double>(frequency);
            
            if (remainingMs > 2.0) 
            {
                SDL_Delay(static_cast<Uint32>(remainingMs - 1.0)); 
            }
            
            while ((SDL_GetPerformanceCounter() - lastCounter_) < targetTicks)
            {
                SDL_DelayNS(0);
            }
        }
    }

    void Timer::Tick()
    {
        const Uint64 currentCounter = SDL_GetPerformanceCounter();
        const Uint64 counterDelta = currentCounter - lastCounter_;
        
        deltaTime_ = static_cast<float>(static_cast<double>(counterDelta) / static_cast<double>(frequency_));
        
        deltaTime_ = std::clamp(deltaTime_, 0.0f, 0.25f);

        lastCounter_ = currentCounter;

        realTimeSinceStartup_ += deltaTime_;
        totalTime_ += deltaTime_ * timeScale_;

        frameCount_++;
        fpsTimer_ += deltaTime_;

        if (fpsTimer_ >= 1.0f)
        {
            fps_ = static_cast<float>(frameCount_) / fpsTimer_;
            frameCount_ = 0;
            fpsTimer_ -= 1.0f;
        }
    }
}
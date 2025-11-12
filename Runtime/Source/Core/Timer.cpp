#include "Core/Timer.h"
#include <SDL3/SDL_timer.h>
#include <algorithm>

#include "Core/Math/Vector2.h"

namespace BixEngine::Core
{
    void Timer::Tick()
    {
        const Uint64 current = SDL_GetPerformanceCounter();
        static const float invFrequency = 1.0f / static_cast<float>(SDL_GetPerformanceFrequency());

        if (lastCounter_ == 0)
        {
            deltaTime_ = 0.0f;   
        }
        else
        {
            deltaTime_ = std::clamp(static_cast<float>(current - lastCounter_) * invFrequency, 0.0f, 0.25f);   
        }
        
        totalTime_ += deltaTime_;
        lastCounter_ = current;
    }

    void Timer::SleepUntilNextFrame(float targetDelta)
    {
        if (deltaTime_ < targetDelta)
        {
            const float remaining = targetDelta - deltaTime_;
            SDL_Delay(static_cast<Uint32>(remaining * 1000.0f));
        }
    }
}

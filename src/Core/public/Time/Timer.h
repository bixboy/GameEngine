#pragma once
#include <SDL3/SDL_stdinc.h>

namespace BixEngine::Core
{
    class Timer
    {
    public:
        Timer();

        void Tick();

        void Reset();
        
        // Temps écoulé entre la frame précédente et l'actuelle (Secondes)
        [[nodiscard]] float GetDeltaTime() const noexcept { return deltaTime_ * timeScale_; }

        // Temps réel écoulé (Non affecté par le TimeScale)
        [[nodiscard]] float GetUnscaledDeltaTime() const noexcept { return deltaTime_; }

        // Temps total écoulé depuis le début du jeu (Scale appliqué)
        [[nodiscard]] float GetTotalTime() const noexcept { return totalTime_; }

        // Temps total réel depuis le lancement (Scale ignoré)
        [[nodiscard]] float GetRealTimeSinceStartup() const noexcept { return realTimeSinceStartup_; }

        // Time Scale (Ralenti / Accéléré)
        void SetTimeScale(float scale) noexcept { timeScale_ = scale >= 0.0f ? scale : 0.0f; }
        [[nodiscard]] float GetTimeScale() const noexcept { return timeScale_; }
        
        // FPS instantané
        [[nodiscard]] float GetInstantFPS() const noexcept { return deltaTime_ > 0.0f ? 1.0f / deltaTime_ : 0.0f; }
        
        // FPS moyen sur la dernière seconde
        [[nodiscard]] float GetFPS() const noexcept { return fps_; }

        // Dors pour atteindre le targetDeltaTime (ex: 16.6ms pour 60fps)
        void SleepUntilNextFrame(float targetDeltaTime);

    private:
        Uint64 frequency_{0};
        Uint64 lastCounter_{0};

        // Delta time
        float deltaTime_{0.0f};
        float totalTime_{0.0f};
        float realTimeSinceStartup_{0.0f};
        
        float timeScale_{1.0f};

        // Fps
        float fps_{0.0f};
        float fpsTimer_{0.0f};
        int frameCount_{0};
    };
}
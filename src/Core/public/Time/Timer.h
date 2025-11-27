#pragma once
#include <SDL3/SDL_stdinc.h>


namespace BixEngine::Core
{
    // Gère le temps entre les frames et le framerate global du moteur.
    class Timer
    {
    public:
        // Met à jour le delta time depuis la dernière frame.
        void Tick();


        // Endort le thread pour limiter la fréquence d’images.
        // Exemple : SleepUntilNextFrame(1.0f / 120.0f) → 120 FPS max.
        void SleepUntilNextFrame(float targetDelta);


        // Définit un facteur de vitesse pour le temps (ralenti/accéléré).
        void SetTimeScale(float scale) noexcept { timeScale_ = scale; }


        // Temps écoulé depuis la dernière frame (en secondes).
        [[nodiscard]] float GetDeltaTime() const noexcept { return deltaTime_; }

        // Delta time affecté par le facteur de temps.
        [[nodiscard]] float GetScaledDeltaTime() const noexcept { return deltaTime_ * timeScale_; }

        // Images par seconde calculées à partir du delta time.
        [[nodiscard]] float GetFPS() const noexcept { return deltaTime_ > 0.0f ? 1.0f / deltaTime_ : 0.0f; }

        // Temps total écoulé depuis le lancement du moteur (en secondes).
        [[nodiscard]] float GetTotalTime() const noexcept { return totalTime_; }

        // Retourne le facteur de temps actuel.
        [[nodiscard]] float GetTimeScale() const noexcept { return timeScale_; }

    private:
        Uint64 lastCounter_{0};
        float deltaTime_{0.0f};
        float totalTime_{0.0f};
        float timeScale_{1.0f};
    };
}

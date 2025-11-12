#pragma once

#include <SDL3/SDL_events.h>
#include <unordered_set>
#include <bitset>
#include <utility>

namespace BixEngine::Input
{
    struct MouseStatistics
    {
        int eventsPerSecond{0};
        int droppedEventsPerSecond{0};
        int processedEventsThisFrame{0};
        int droppedEventsThisFrame{0};
    };

    class Input
    {
    public:
        Input() = default;

        // Traite un événement SDL
        void ProcessEvent(const SDL_Event& event);

        // Met à jour l'état des entrées (à appeler une fois par frame)
        void PostUpdate() noexcept;

        void UpdateStatistics(float deltaTime) noexcept;
        void NotifyMouseEventDropped() noexcept;

        // Réinitialise tous les états d'entrée (utilisé quand on quitte le viewport)
        void ResetState() noexcept;

        // ============================
        // 🔹 Clavier
        // ============================
        
        [[nodiscard]] bool IsKeyDown(SDL_Keycode key) const noexcept;
        [[nodiscard]] bool WasKeyPressed(SDL_Keycode key) const noexcept;
        [[nodiscard]] bool WasKeyReleased(SDL_Keycode key) const noexcept;

        // ============================
        // 🔹 Souris
        // ============================
        
        [[nodiscard]] bool IsMouseButtonDown(int button) const noexcept;
        [[nodiscard]] bool WasMouseButtonPressed(int button) const noexcept;
        [[nodiscard]] bool WasMouseButtonReleased(int button) const noexcept;
        [[nodiscard]] std::pair<float, float> GetMouseDelta() const noexcept;
        [[nodiscard]] std::pair<int, int> GetMouseWheel() const noexcept;

        [[nodiscard]] const MouseStatistics& GetMouseStatistics() const noexcept { return mouseStats_; }

        // ============================
        // 🔹 Système
        // ============================
        
        [[nodiscard]] bool IsQuitRequested() const noexcept { return quitRequested_; }

    private:
        // ============================
        // Clavier
        // ============================
        
        std::unordered_set<SDL_Keycode> heldKeys_{};
        std::unordered_set<SDL_Keycode> pressedKeys_{};
        std::unordered_set<SDL_Keycode> releasedKeys_{};

        // ============================
        // Souris
        // ============================
        
        static constexpr size_t kMaxMouseButtons = 8;
        std::bitset<kMaxMouseButtons> mouseButtons_{};
        std::bitset<kMaxMouseButtons> mouseButtonsPressed_{};
        std::bitset<kMaxMouseButtons> mouseButtonsReleased_{};

        std::pair<float, float> mouseDelta_{0.0f, 0.0f};
        std::pair<float, float> mouseWheel_{0.0f, 0.0f};

        MouseStatistics mouseStats_{};
        int mouseEventsProcessedFrame_{0};
        int mouseEventsDroppedFrame_{0};
        int mouseEventsAccumulated_{0};
        int mouseEventsDroppedAccumulated_{0};
        float mouseStatisticsTimer_{0.0f};
    
        // ============================
        // Système
        // ============================

        bool quitRequested_{false};
    };
}

#pragma once

#include <SDL3/SDL_events.h>
#include <unordered_set>
#include <bitset>
#include <utility>

namespace BixEngine::Input
{
    class Input
    {
    public:
        Input() = default;

        // Traite un événement SDL
        void ProcessEvent(const SDL_Event& event);

        // Met à jour l'état des entrées (à appeler une fois par frame)
        void PostUpdate() noexcept;

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

        // ============================
        // Système
        // ============================
        
        bool quitRequested_{false};
    };
}

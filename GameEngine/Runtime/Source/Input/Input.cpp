#include "Input/Input.h"

namespace BixEngine::Input
{
    void Input::ProcessEvent(const SDL_Event& event)
    {
        switch (event.type)
        {
            
        case SDL_EVENT_QUIT:
            quitRequested_ = true;
            break;

        // ----------------------------
        // Clavier
        // ----------------------------
        case SDL_EVENT_KEY_DOWN:
            if (!event.key.repeat)
            {
                pressedKeys_.insert(event.key.key);
                heldKeys_.insert(event.key.key);
            }
            break;

        case SDL_EVENT_KEY_UP:
            heldKeys_.erase(event.key.key);
            releasedKeys_.insert(event.key.key);
            break;

        // ----------------------------
        // Souris
        // ----------------------------
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button < kMaxMouseButtons)
            {
                mouseButtons_.set(event.button.button, true);
                mouseButtonsPressed_.set(event.button.button, true);
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button < kMaxMouseButtons)
            {
                mouseButtons_.set(event.button.button, false);
                mouseButtonsReleased_.set(event.button.button, true);
            }
            break;

        case SDL_EVENT_MOUSE_MOTION:
            mouseDelta_.first  += static_cast<float>(event.motion.xrel);
            mouseDelta_.second += static_cast<float>(event.motion.yrel);
            break;

        case SDL_EVENT_MOUSE_WHEEL:
            mouseWheel_.first  += event.wheel.x;
            mouseWheel_.second += event.wheel.y;
            break;

        default:
            break;
        }
    }

    void Input::PostUpdate() noexcept
    {
        pressedKeys_.clear();
        releasedKeys_.clear();
        mouseButtonsPressed_.reset();
        mouseButtonsReleased_.reset();
        mouseDelta_ = {0.0f, 0.0f};
        mouseWheel_ = {0, 0};
    }

    void Input::ResetState() noexcept
    {
        heldKeys_.clear();
        pressedKeys_.clear();
        releasedKeys_.clear();

        mouseButtons_.reset();
        mouseButtonsPressed_.reset();
        mouseButtonsReleased_.reset();

        mouseDelta_ = {0.0f, 0.0f};
        mouseWheel_ = {0, 0};

        quitRequested_ = false;
    }

    // ============================
    // Clavier
    // ============================
    bool Input::IsKeyDown(SDL_Keycode key) const noexcept
    {
        return heldKeys_.contains(key);
    }

    bool Input::WasKeyPressed(SDL_Keycode key) const noexcept
    {
        return pressedKeys_.contains(key);
    }

    bool Input::WasKeyReleased(SDL_Keycode key) const noexcept
    {
        return releasedKeys_.contains(key);
    }

    // ============================
    // Souris
    // ============================
    
    bool Input::IsMouseButtonDown(int button) const noexcept
    {
        return button >= 0 && static_cast<size_t>(button) < kMaxMouseButtons && mouseButtons_.test(static_cast<size_t>(button));
    }

    bool Input::WasMouseButtonPressed(int button) const noexcept
    {
        return button >= 0 && static_cast<size_t>(button) < kMaxMouseButtons && mouseButtonsPressed_.test(static_cast<size_t>(button));
    }

    bool Input::WasMouseButtonReleased(int button) const noexcept
    {
        return button >= 0 && static_cast<size_t>(button) < kMaxMouseButtons && mouseButtonsReleased_.test(static_cast<size_t>(button));
    }

    std::pair<float, float> Input::GetMouseDelta() const noexcept
    {
        return mouseDelta_;
    }

    std::pair<int, int> Input::GetMouseWheel() const noexcept
    {
        return mouseWheel_;
    }
}

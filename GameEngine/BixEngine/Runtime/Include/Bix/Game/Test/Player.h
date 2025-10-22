#pragma once
#include <SDL3/SDL.h>
#include <memory>
#include "Bix/Game/Actor.h"

namespace BixEngine
{
    namespace Graphics { class Renderer; }
    namespace Input    { class InputManager; }

    namespace Game
    {
        class SpriteComponent;

        BCLASS(Player, Actor, Actor)
        class Player : public Actor
        {
            public:
                BIX_GENERATED_BODY();

                Player();
                Player(const Math::Vector3& position, const Math::Vector3& size, SDL_Color color);

                void SetupInput(Input::InputManager& inputManager);

                void Update(float deltaTime) override;

                void MoveForward(float value);
                void MoveRight(float value);

                [[nodiscard]] String GetTypeName() const noexcept override { return "Player"; }
                [[nodiscard]] std::unique_ptr<Actor> ClonePrototype() const override { return std::make_unique<Player>(); }

            protected:
                static void RegisterProperties(::BixEngine::Engine::SaveSystem::BixClass& cls);

            private:
                void OnPostDeserialize() override;
                void OnComponentRemoved(const Component& component) override;

                void ApplyMovement(float deltaTime);
                void InitializeSpriteComponent();
                void RefreshSpriteComponent();

                Math::Vector2 pendingInput_{};

                BPROPERTY()
                float moveSpeed_ = 200.0f;

                BPROPERTY()
                Math::Vector3 size_ = Math::Vector3(32.0f, 32.0f, 1.0f);

                BPROPERTY()
                SDL_Color color_ = SDL_Color{255, 255, 255, 255};
                SpriteComponent* spriteComponent_{nullptr};
        };
    }
}


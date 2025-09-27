#include "Game/Test/Player.h"

#include <istream>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>

#include "Game/Components/SpriteComponent.h"
#include "Game/SceneSerializer.h"
#include "Input/InputManager.h"
#include "Math/Math.h"

namespace Engine::Game
{
    namespace
    {
        struct PlayerFactoryRegistration
        {
            PlayerFactoryRegistration()
            {
                SceneSerializer::RegisterActorFactory("Player", []()
                {
                    return std::make_unique<Player>();
                });
            }
        };

        static PlayerFactoryRegistration g_playerFactoryRegistration;
    }

    Player::Player()
        : Actor("Player")
    {
        InitializeSpriteComponent();
    }

    Player::Player(const Math::Vector3& position, const Math::Vector3& size, SDL_Color color)
        : Actor(Math::Transform(position, Math::Rotator(), size))
    {
        size_ = size;
        color_ = color;
        InitializeSpriteComponent();
    }

    void Player::SetupInput(Input::InputManager& inputManager)
    {
        inputManager.BindAxis("MoveForward", SDLK_W, this, &Player::MoveForward, 1.0f);
        inputManager.BindAxis("MoveForward", SDLK_Z, this, &Player::MoveForward, 1.0f);
        inputManager.BindAxis("MoveForward", SDLK_UP, this, &Player::MoveForward, 1.0f);
        inputManager.BindAxis("MoveForward", SDLK_S, this, &Player::MoveForward, -1.0f);
        inputManager.BindAxis("MoveForward", SDLK_DOWN, this, &Player::MoveForward, -1.0f);

        inputManager.BindAxis("MoveRight", SDLK_D, this, &Player::MoveRight, 1.0f);
        inputManager.BindAxis("MoveRight", SDLK_RIGHT, this, &Player::MoveRight, 1.0f);
        inputManager.BindAxis("MoveRight", SDLK_A, this, &Player::MoveRight, -1.0f);
        inputManager.BindAxis("MoveRight", SDLK_Q, this, &Player::MoveRight, -1.0f);
        inputManager.BindAxis("MoveRight", SDLK_LEFT, this, &Player::MoveRight, -1.0f);

        inputManager.UnbindAxis("MoveRight", SDLK_W);
    }

    void Player::Update(float deltaTime)
    {
        ApplyMovement(deltaTime);
    }

    void Player::MoveForward(float value)
    {
        pendingInput_.y = value;
    }

    void Player::MoveRight(float value)
    {
        pendingInput_.x = value;
    }

    void Player::ApplyMovement(float deltaTime)
    {
        Math::Vector2 input = pendingInput_;
        if (input.LengthSquared() > 1.0f)
            input = input.Normalized();

        Math::Vector3 movement{ input.x, -input.y, 0.0f };
        movement = movement * (moveSpeed_ * deltaTime);

        SetPosition(GetPosition() + movement);
    }

    void Player::SerializeJsonImpl(nlohmann::json& json) const
    {
        json["moveSpeed"] = moveSpeed_;
        json["color"] = {
            {"r", color_.r},
            {"g", color_.g},
            {"b", color_.b},
            {"a", color_.a}
        };
        json["size"] = {
            {"x", size_.x},
            {"y", size_.y},
            {"z", size_.z}
        };
    }

    void Player::DeserializeJsonImpl(const nlohmann::json& json)
    {
        moveSpeed_ = json.value("moveSpeed", moveSpeed_);

        if (json.contains("color"))
        {
            const auto& colorJson = json["color"];
            color_.r = static_cast<Uint8>(colorJson.value("r", static_cast<int>(color_.r)));
            color_.g = static_cast<Uint8>(colorJson.value("g", static_cast<int>(color_.g)));
            color_.b = static_cast<Uint8>(colorJson.value("b", static_cast<int>(color_.b)));
            color_.a = static_cast<Uint8>(colorJson.value("a", static_cast<int>(color_.a)));
        }

        if (json.contains("size"))
        {
            const auto& sizeJson = json["size"];
            size_.x = sizeJson.value("x", size_.x);
            size_.y = sizeJson.value("y", size_.y);
            size_.z = sizeJson.value("z", size_.z);
        }

        RefreshSpriteComponent();
        SetScale(size_);
    }

    void Player::SerializeBinaryImpl(std::ostream& stream) const
    {
        WritePrimitive(stream, moveSpeed_);
        stream.write(reinterpret_cast<const char*>(&color_), sizeof(color_));
        WritePrimitive(stream, size_.x);
        WritePrimitive(stream, size_.y);
        WritePrimitive(stream, size_.z);
    }

    void Player::DeserializeBinaryImpl(std::istream& stream)
    {
        ReadPrimitive(stream, moveSpeed_);
        stream.read(reinterpret_cast<char*>(&color_), sizeof(color_));
        if (!stream)
            throw std::runtime_error("Failed to read player color from stream.");
        ReadPrimitive(stream, size_.x);
        ReadPrimitive(stream, size_.y);
        ReadPrimitive(stream, size_.z);

        RefreshSpriteComponent();
        SetScale(size_);
    }

    void Player::InitializeSpriteComponent()
    {
        auto sprite = std::make_unique<SpriteComponent>(this, color_, size_.x, size_.y);
        spriteComponent_ = sprite.get();
        AddComponent(std::move(sprite));
        SetScale(size_);
    }

    void Player::RefreshSpriteComponent()
    {
        if (!spriteComponent_)
        {
            InitializeSpriteComponent();
            return;
        }

        spriteComponent_->SetColor(color_);
        spriteComponent_->SetDimensions(size_.x, size_.y);
    }
}


#include "Game/Actor.h"
#include "Game/Components/Component.h"


namespace Engine::Game
{
    Actor::Actor(const Math::Transform& transform)
        : transform_(transform) {}

    Actor::~Actor() = default;

    void Actor::Update(float deltaTime)
    {
        for (auto& comp : components_)
        {
            comp->Update(deltaTime);
        }
    }

    void Actor::Render(Graphics::Renderer& renderer) const
    {
        for (const auto& comp : components_)
        {
            comp->Render(renderer);
        }
    }

    void Actor::AddComponent(std::unique_ptr<Component> component)
    {
        components_.push_back(std::move(component));
    }
}


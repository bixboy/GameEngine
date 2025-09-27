#include "Game/Actor.h"
#include "Game/Components/Component.h"

#include <utility>


namespace Engine::Game
{
    Actor::Actor(const Math::Transform& transform)
        : Object("Actor", transform) {}

    Actor::Actor(std::string name, const Math::Transform& transform)
        : Object(std::move(name), transform) {}

    Actor::~Actor() = default;

    void Actor::BeginPlay()
    {
        for (auto& c : components_)
            c->BeginPlay();
    }

    void Actor::Update(float deltaTime)
    {
        if (!hasBegunPlay_)
        {
            BeginPlay();
            hasBegunPlay_ = true;
        }
        
        for (auto& comp : components_)
            comp->Update(deltaTime);
    }

    void Actor::Render(Graphics::Renderer& renderer) const
    {
        for (const auto& comp : components_)
            comp->Render(renderer);
    }

    void Actor::AddComponent(std::unique_ptr<Component> component)
    {
        components_.push_back(std::move(component));
    }
}


#include "Game/Actor.h"
#include "Game/Components/Component.h"

#include <memory>
#include <utility>


namespace Engine::Game
{
    Actor::Actor(const Math::Transform& transform) : Object("Actor", transform) {}

    Actor::Actor(String name, const Math::Transform& transform) : Object(std::move(name), transform) {}
    
    void Actor::BeginPlay()
    {
        for (auto& c : components_)
            c->BeginPlay();
    }

    void Actor::Update(float deltaTime)
    {
        if (!has_begun_play_)
        {
            BeginPlay();
            has_begun_play_ = true;
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

    std::unique_ptr<Actor> Actor::ClonePrototype() const
    {
        return std::make_unique<Actor>();
    }
}


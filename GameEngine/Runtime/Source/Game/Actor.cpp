#include "Game/Actor.h"
#include "Game/Components/Component.h"

#include <algorithm>
#include <memory>
#include <utility>


namespace BixEngine::Game
{
    Actor::Actor(const Math::Transform& transform) : Object("Actor", transform)
    {
        
    }

    Actor::Actor(String name, const Math::Transform& transform) : Object(std::move(name), transform)
    {
        
    }
    
    void Actor::BeginPlay()
    {
        for (auto& c : components_)
            c->BeginPlay();
    }

    void Actor::Update(float deltaTime)
    {
        if (!active_)
        {
            return;
        }

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
        if (!active_)
        {
            return;
        }

        for (const auto& comp : components_)
            comp->Render(renderer);
    }

    void Actor::AddComponent(std::unique_ptr<Component> component)
    {
        components_.push_back(std::move(component));
    }

    bool Actor::RemoveComponent(const Component* component)
    {
        if (!component)
        {
            return false;
        }

        const auto it = std::find_if(components_.begin(), components_.end(), [component](const std::unique_ptr<Component>& entry)
        {
            return entry.get() == component;
        });

        if (it == components_.end())
        {
            return false;
        }

        OnComponentRemoved(*(*it));
        components_.erase(it);
        return true;
    }

    std::unique_ptr<Actor> Actor::ClonePrototype() const
    {
        return std::make_unique<Actor>();
    }
}


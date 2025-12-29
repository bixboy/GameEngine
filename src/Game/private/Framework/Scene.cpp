#include "Framework/Scene.h"
#include "Framework/Actor.h"
#include "Debug/Logger.h"
#include "Gui/Core/GuiManager.h"

namespace BixEngine::Game
{
    Scene::Scene(String name)
        : name_(std::move(name))
    {
        LOG_INFO("Scene created: " + name_);
    }

    void Scene::SetName(String name)
    {
        name_ = std::move(name);
    }

    void Scene::SetSourcePath(const String& path)
    {
        sourcePath_ = path;
    }

    void Scene::OnRuntimeStart()
    {
        
        if (!b2World_IsValid(physicsWorldId_))
        {
            b2WorldDef worldDef = b2DefaultWorldDef();
            worldDef.gravity = {0.0f, 10.0f * 3.5f}; 
            
            
            worldDef.gravity = {0.0f, 9.8f}; 
            physicsWorldId_ = b2CreateWorld(&worldDef);
            LOG_INFO("Physics World Created.");
        }

        
        
        for (size_t i = 0; i < actors_.size(); ++i)
        {
            if(actors_[i]) actors_[i]->BeginPlay();
        }
    }

    void Scene::OnRuntimeStop()
    {
        
        
        
        
        
        if (b2World_IsValid(physicsWorldId_))
        {
            b2DestroyWorld(physicsWorldId_);
            physicsWorldId_ = b2_nullWorldId;
            LOG_INFO("Physics World Destroyed.");
        }
    }

    void Scene::OnEditorUpdate(float deltaTime)
    {
        
        (void)deltaTime;

        
         while (!pendingDestruction_.empty())
        {
            std::vector<std::unique_ptr<Actor>> batch = std::move(pendingDestruction_);
            pendingDestruction_.clear(); 
            batch.clear(); 
        }

        std::erase_if(actors_, [](const std::unique_ptr<Actor>& ptr) { return !ptr; });
    }

    void Scene::OnRuntimeUpdate(float deltaTime)
    {
        
        if (b2World_IsValid(physicsWorldId_))
        {
            constexpr float timeStep = 1.0f / 60.0f;
            constexpr int subStepCount = 4;
            
            physicsAccumulator_ += deltaTime;
            
            
            if (physicsAccumulator_ > 0.2f) physicsAccumulator_ = 0.2f;

            while (physicsAccumulator_ >= timeStep)
            {
                b2World_Step(physicsWorldId_, timeStep, subStepCount);
                physicsAccumulator_ -= timeStep;
            }
        }

        
        
        for (size_t i = 0; i < actors_.size(); ++i)
        {
            if(actors_[i] && actors_[i]->IsActive()) 
                actors_[i]->Update(deltaTime);
        }

        
        
        
        while (!pendingDestruction_.empty())
        {
            
            std::vector<std::unique_ptr<Actor>> batch = std::move(pendingDestruction_);
            pendingDestruction_.clear(); 
            batch.clear(); 
        }

        
        
        std::erase_if(actors_, [](const std::unique_ptr<Actor>& ptr) { return !ptr; });
    }

    void Scene::SetContext(SceneContext context) noexcept
    {
        context_ = context;
    }

    void Scene::AddActor(std::unique_ptr<Actor> actor)
    {
        if (actor)
        {
            actor->SetOwningScene(this);
            actor->SetParent(nullptr); 
            actors_.push_back(std::move(actor));
        }
    }

    void Scene::RemoveActor(Actor* actor)
    {
        if (!actor) return;
        
        
        std::vector<Actor*> toRemove;
        toRemove.push_back(actor);

        
        size_t head = 0;
        while(head < toRemove.size())
        {
            Actor* current = toRemove[head++];
            for(auto* child : current->GetChildren())
            {
                toRemove.push_back(child);
            }
        }

        
        for(Actor* a : toRemove)
        {
            auto it = std::find_if(actors_.begin(), actors_.end(), 
                [&](const auto& ptr){ return ptr.get() == a; });
                
            if (it != actors_.end())
            {
                
                
                pendingDestruction_.push_back(std::move(*it));
            }
        }
    }

    void Scene::ClearActors() noexcept
    {
        actors_.clear();
    }

    Actor* Scene::FindActorByName(const String& name) noexcept
    {
        for (auto& actor : actors_)
        {
            if (actor && actor->GetName() == name)
                return actor.get();
        }
        return nullptr;
    }

    Actor* Scene::FindActorByPath(const String& path) noexcept
    {
         for (auto& actor : actors_)
        {
            
             if (actor && actor->GetName() == path)
                return actor.get();
        }
        return nullptr;
    }

    void Scene::Rename(String name)
    {
        SetName(std::move(name));
    }

    Core::Window& Scene::GetWindow() const
    {
        
        return *context_.window;
    }

    Input::InputManager& Scene::GetInputManager() const
    {
        return *context_.inputManager;
    }

    Graphics::Renderer& Scene::GetRenderer() const
    {
        return *context_.renderer;
    }

    Core::Timer& Scene::GetTimer() const
    {
        return *context_.timer;
    }

    Gui::GuiManager& Scene::GetGuiManager() const
    {
        return *context_.guiManager;
    }

    void Scene::Render(Graphics::Renderer& renderer)
    {
        for (auto& actor : actors_)
        {
            if (actor && actor->IsActive())
            {
                actor->Render(renderer);
            }
        }
    }

    void Scene::PostRender(Graphics::Renderer& renderer)
    {
        
        
        
        
        
        
        (void)renderer;
    }
}

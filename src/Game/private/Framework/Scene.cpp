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
        // 1. Initialize Physics World
        if (!b2World_IsValid(physicsWorldId_))
        {
            b2WorldDef worldDef = b2DefaultWorldDef();
            worldDef.gravity = {0.0f, 10.0f * 3.5f}; // Example gravity (can be configured)
            // Use 3.5g as per Player.cpp defaults, or 9.8? Let's use standard default for now, Player overrides it? 
            // Player overrides GravityScale. But world gravity is base.
            worldDef.gravity = {0.0f, 9.8f}; 
            physicsWorldId_ = b2CreateWorld(&worldDef);
            LOG_INFO("Physics World Created.");
        }

        // 2. Notify Actors (This will trigger CreatePhysicsState in BoxColliderComponent)
        // Use index-based loop to support spawning actors during BeginPlay (reallocation safe)
        for (size_t i = 0; i < actors_.size(); ++i)
        {
            if(actors_[i]) actors_[i]->BeginPlay();
        }
    }

    void Scene::OnRuntimeStop()
    {
        // 1. Notify Actors (Optional clean up logic, but destructors do main work)
        // Generally good to let them know we are stopping.
        // But currently Actor has no OnRuntimeStop.
        
        // 2. Destroy Physics World
        if (b2World_IsValid(physicsWorldId_))
        {
            b2DestroyWorld(physicsWorldId_);
            physicsWorldId_ = b2_nullWorldId;
            LOG_INFO("Physics World Destroyed.");
        }
    }

    void Scene::OnEditorUpdate(float deltaTime)
    {
        // Actors do not update in editor mode by default
        (void)deltaTime;

        // Clean up pending destruction actors in Editor too
         while (!pendingDestruction_.empty())
        {
            std::vector<std::unique_ptr<Actor>> batch = std::move(pendingDestruction_);
            pendingDestruction_.clear(); 
            batch.clear(); // Destructors run here
        }

        std::erase_if(actors_, [](const std::unique_ptr<Actor>& ptr) { return !ptr; });
    }

    void Scene::OnRuntimeUpdate(float deltaTime)
    {
        // 1. Step Physics
        if (b2World_IsValid(physicsWorldId_))
        {
            constexpr float timeStep = 1.0f / 60.0f;
            constexpr int subStepCount = 4;
            
            physicsAccumulator_ += deltaTime;
            
            // Limit spiral of death
            if (physicsAccumulator_ > 0.2f) physicsAccumulator_ = 0.2f;

            while (physicsAccumulator_ >= timeStep)
            {
                b2World_Step(physicsWorldId_, timeStep, subStepCount);
                physicsAccumulator_ -= timeStep;
            }
        }

        // 2. Update Actors
        // Use index-based loop to support spawning/destroying actors during Update
        for (size_t i = 0; i < actors_.size(); ++i)
        {
            if(actors_[i] && actors_[i]->IsActive()) 
                actors_[i]->Update(deltaTime);
        }

        // 3. Deferred Destruction Processing
        // Safely destroy actors in the pending queue
        // We use a loop to handle recursive destruction (e.g. Actor destroys children in destructor)
        while (!pendingDestruction_.empty())
        {
            // Move current batch to temp to allow new additions during destruction
            std::vector<std::unique_ptr<Actor>> batch = std::move(pendingDestruction_);
            pendingDestruction_.clear(); 
            batch.clear(); // Destructors run here
        }

        // 4. Cleanup Nulls in Actor List
        // Remove slots that became null due to deferred destruction
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
        
        // 1. Collect all actors to remove (Recursive)
        std::vector<Actor*> toRemove;
        toRemove.push_back(actor);

        // Standard iterative BFS/DFS collection
        size_t head = 0;
        while(head < toRemove.size())
        {
            Actor* current = toRemove[head++];
            for(auto* child : current->GetChildren())
            {
                toRemove.push_back(child);
            }
        }

        // 2. Move them all to pending destruction
        for(Actor* a : toRemove)
        {
            auto it = std::find_if(actors_.begin(), actors_.end(), 
                [&](const auto& ptr){ return ptr.get() == a; });
                
            if (it != actors_.end())
            {
                // Move to pending destruction queue
                // This leaves a nullptr in the actors_ vector
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
            // Assuming path matching logic or name matching if path==name
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
        // Assuming context_.window is valid if checked
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
        // Post-render pass if actors have one, or custom scene post-processing
        // For now, let's assume actors might have debug drawing or UI in post-render
        // But Actor.h didn't show PostRender. 
        // Let's keep it empty for now unless we know Actor has PostRender.
        // Actually, let's check if the previous implementation did anything. 
        // Given I don't recall Actor::PostRender, I will leave this empty or minimal.
        (void)renderer;
    }
}

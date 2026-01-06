#include "Framework/Scene.h"
#include "Framework/Actor.h"
#include "Debug/Logger.h"
#include "Gui/Core/GuiManager.h"
#include "Components/Core/BoxColliderComponent.h"
#include "Components/Core/CameraComponent.h"
#include "Framework/CollisionHitResult.h"


namespace BixEngine::Game
{
    namespace
    {
        Actor* FindActorByNameRecursive(Actor* current, const String& name)
        {
            if (current->GetName() == name)
                return current;

            for (auto* child : current->GetChildren())
            {
                if (auto* found = FindActorByNameRecursive(child, name))
                    return found;
            }
            
            return nullptr;
        }

        Actor* FindActorByIDRecursive(Actor* current, const String& uuid)
        {
            if (current->GetUUID() == uuid)
                return current;

            for (auto* child : current->GetChildren())
            {
                if (auto* found = FindActorByIDRecursive(child, uuid))
                    return found;
            }
            
            return nullptr;
        }
    }

    Scene::Scene(String name) : name_(std::move(name))
    {
        LOG_INFO("Scene created: " + name_);
    }

    Scene::~Scene()
    {
        OnRuntimeStop();
        ClearActors();
    }

    void Scene::SetName(String name)
    {
        name_ = std::move(name);
    }

    void Scene::SetSourcePath(const String& path)
    {
        sourcePath_ = path;
    }

    // --- Events Système ---

    void Scene::OnWindowResize(int width, int height)
    {
        if (activeCamera_)
        {
            activeCamera_->SetAspectRatio(static_cast<float>(width) / static_cast<float>(height));
        }
    }

    void Scene::OnFileDrop(const String& filePath)
    {
        LOG_INFO("File dropped in scene: " + filePath);
        
        std::filesystem::path path(filePath.c_str());
        String extension = path.extension().string();
        
        // Simple extension check
        if (extension == ".prefab")
        {
             // TODO: Spawn Actor from Prefab
             LOG_INFO("Prefab drop detected. Spawning logic to be implemented.");
        }
        else if (extension == ".png" || extension == ".jpg" || extension == ".tga")
        {
             LOG_INFO("Texture drop detected. Import logic to be implemented.");
        }
        else if (extension == ".scene")
        {
             LOG_INFO("Scene drop detected. Loading logic to be implemented.");
        }
    }

    void Scene::HandleEvent(const SDL_Event& event) { }

    // --- Runtime ---

    void Scene::OnRuntimeStart()
    {
        if (!b2World_IsValid(physicsWorldId_))
        {
            b2WorldDef worldDef = b2DefaultWorldDef();
            worldDef.gravity = {0.0f, 9.81f};
            physicsWorldId_ = b2CreateWorld(&worldDef);
            LOG_INFO("Physics World Created.");
        }

        for (auto& actor : actors_)
        {
            if(actor)
                actor->BeginPlay();
        }
    }

    void Scene::OnRuntimeStop()
    {
        for (auto& actor : actors_)
        {
            if(actor)
                actor->EndPlay();
        }
        
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
        
        pendingDestruction_.clear(); 
        std::erase_if(actors_, [](const std::unique_ptr<Actor>& ptr) { return !ptr; });
    }

    void Scene::OnRuntimeUpdate(float deltaTime)
    {
        // --- Physique Box2D ---
        if (b2World_IsValid(physicsWorldId_))
        {
            constexpr float timeStep = 1.0f / 60.0f;
            constexpr int subStepCount = 4;
            
            physicsAccumulator_ += deltaTime;
            physicsAccumulator_ = std::min(physicsAccumulator_, 0.2f); 

            while (physicsAccumulator_ >= timeStep)
            {
                b2World_Step(physicsWorldId_, timeStep, subStepCount);
                physicsAccumulator_ -= timeStep;
            }

            // Gestion des collisions
            b2ContactEvents events = b2World_GetContactEvents(physicsWorldId_);
            for (int i = 0; i < events.beginCount; ++i)
            {
                b2ContactBeginTouchEvent* beginEvent = events.beginEvents + i;
                
                b2BodyId bodyA = b2Shape_GetBody(beginEvent->shapeIdA);
                b2BodyId bodyB = b2Shape_GetBody(beginEvent->shapeIdB);

                auto* colA = static_cast<BoxColliderComponent*>(b2Body_GetUserData(bodyA));
                auto* colB = static_cast<BoxColliderComponent*>(b2Body_GetUserData(bodyB));

                if (colA && colB)
                {
                    CollisionHitResult resultA;
                    resultA.OtherActor = colB->GetOwner();
                    colA->DispatchCollisionEnter(resultA.OtherActor, resultA);

                    CollisionHitResult resultB;
                    resultB.OtherActor = colA->GetOwner();
                    colB->DispatchCollisionEnter(resultB.OtherActor, resultB);
                }
            }
        }

        for (auto& actor : actors_)
        {
            if(actor && actor->IsActive()) 
                actor->Update(deltaTime);
        }

        pendingDestruction_.clear();
        std::erase_if(actors_, [](const std::unique_ptr<Actor>& ptr) { return !ptr; });
    }

    // --- Gestion Acteurs ---

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
        if (!actor)
            return;
        
        if (actor->IsPendingKill())
            return;
        
        actor->MarkAsPendingKill();

        auto it = std::ranges::find(actors_, actor, [](const auto& ptr) { return ptr.get(); });
        
        if (it != actors_.end())
        {
            pendingDestruction_.push_back(std::move(*it));
            return; 
        }

        Actor* parent = actor->GetParent();
        if (parent)
        {
            std::unique_ptr<Actor> dyingChild = parent->RemoveChild(actor);
            if (dyingChild)
            {
                pendingDestruction_.push_back(std::move(dyingChild));
            }
        }
    }

    void Scene::ClearActors() noexcept
    {
        actors_.clear();
        pendingDestruction_.clear();
    }

    // --- Rendering ---

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
        // Gizmos, Debug Draw, UI Overlay spécifique à la scène...
    }

    // --- Recherches ---

    Actor* Scene::FindActorByName(const String& name) noexcept
    {
        for (auto& actor : actors_)
        {
            if (auto* found = FindActorByNameRecursive(actor.get(), name))
                return found;
        }
        
        return nullptr;
    }

    Actor* Scene::FindActorByID(const String& uuid) noexcept
    {
        for (auto& actor : actors_)
        {
            if (auto* found = FindActorByIDRecursive(actor.get(), uuid))
                return found;
        }
        
        return nullptr;
    }

    // --- Getters / Setters ---

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
}
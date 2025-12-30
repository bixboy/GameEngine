#include "Framework/Scene.h"
#include "Framework/Actor.h"
#include "Debug/Logger.h"
#include "Gui/Core/GuiManager.h"
#include "Components/Core/BoxColliderComponent.h"
#include "Components/Core/CameraComponent.h"
#include "Framework/CollisionHitResult.h"

namespace BixEngine::Game
{
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
        LOG_INFO("File dropped in scene: {}", filePath);
        // TODO: Logique pour charger un asset ou spawner un acteur
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
        // Appel de EndPlay ?
        
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
        // Nettoyage des acteurs détruits
        pendingDestruction_.clear(); 
        std::erase_if(actors_, [](const std::unique_ptr<Actor>& ptr) { return !ptr; });
    }

    void Scene::OnRuntimeUpdate(float deltaTime)
    {
        // --- Physique Box2D v3 ---
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
        std::erase_if(actors_,
            [](const std::unique_ptr<Actor>& ptr)
            {
                return !ptr;
            });
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

        // Cas 1 : C'est un actor Racine
        auto it = std::ranges::find(actors_, actor, [](const auto& ptr)
        { 
            return ptr.get(); 
        });
        
        if (it != actors_.end())
        {
            pendingDestruction_.push_back(std::move(*it));
            return; 
        }

        // Cas 2 : C'est un enfant
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
        // 1. Setup Camera (View/Projection)
        /*
        if (activeCamera_)
        {
            activeCamera_->ApplyTransform(renderer);
        }
        */

        // 2. Render Actors
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

    Core::Window& Scene::GetWindow() const { return *context_.window; }
    Input::InputManager& Scene::GetInputManager() const { return *context_.inputManager; }
    Graphics::Renderer& Scene::GetRenderer() const { return *context_.renderer; }
    Core::Timer& Scene::GetTimer() const { return *context_.timer; }
    Gui::GuiManager& Scene::GetGuiManager() const { return *context_.guiManager; }
}
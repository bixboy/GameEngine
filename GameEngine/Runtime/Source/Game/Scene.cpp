#include "Game/Scene.h"

#include <stdexcept>
#include <utility>

#include "Core/Timer.h"
#include "Engine/Systems/Window.h"
#include "Game/SceneSerializer.h"
#include "Graphics/Renderer.h"
#include "Input/Input.h"
#include "Engine/Gui/GuiManager.h"
#include "Core/Logger.h"

namespace BixEngine::Game
{
    Scene::Scene(String name): name_(std::move(name))
    {
        LOG_INFO("Scene created: " + name_);
    }

    void Scene::SetContext(SceneContext context) noexcept
    {
        context_ = context;
    }

    void Scene::AddActor(std::unique_ptr<Actor> actor)
    {
        if (!actor)
            return;

        SceneSerializer::EnsureActorFactory(*actor);
        actor->SetOwningScene(this);
        actors_.push_back(std::move(actor));
    }

    void Scene::RemoveActor(Actor* actor)
    {
        if (!actor) return;

        std::erase_if(actors_, [&](const std::unique_ptr<Actor>& ptr)
        {
            if (ptr.get() == actor)
            {
                actor->SetOwningScene(nullptr);
                return true;
            }
            return false;
        });
    }

    void Scene::ClearActors() noexcept
    {
        for (auto& actor : actors_)
        {
            if (actor)
                actor->SetOwningScene(nullptr);
        }

        actors_.clear();
    }

    Actor* Scene::FindActorByName(const String& name) noexcept
    {
        for (auto& a : actors_)
        {
            if (a && a->GetName() == name)
                return a.get();
        }
        return nullptr;
    }

    void Scene::Rename(String name)
    {
        name_ = std::move(name);
    }

    void Scene::SetName(String name)
    {
        name_ = std::move(name);
    }

    // ────────────────────────────────────────────────
    // 🧠 Accès aux sous-systèmes
    // ────────────────────────────────────────────────
    
    Input::InputManager& Scene::GetInputManager() const
    {
        if (!context_.inputManager)
            throw std::runtime_error("Scene context does not provide an input subsystem.");
        
        return *context_.inputManager;
    }

    Graphics::Renderer& Scene::GetRenderer() const
    {
        if (!context_.renderer)
            throw std::runtime_error("Scene context does not provide a renderer.");
        
        return *context_.renderer;
    }

    Core::Window& Scene::GetWindow() const
    {
        if (!context_.window)
            throw std::runtime_error("Scene context does not provide a window.");
        
        return *context_.window;
    }

    Core::Timer& Scene::GetTimer() const
    {
        if (!context_.timer)
            throw std::runtime_error("Scene context does not provide a timer.");
        
        return *context_.timer;
    }

    Gui::GuiManager& Scene::GetGuiManager() const
    {
        if (!context_.guiManager)
            throw std::runtime_error("Scene context does not provide a GUI manager.");
        
        return *context_.guiManager;
    }
}

#include "Bix/Game/Scene.h"

#include <stdexcept>
#include <utility>

#include <array>

#include "Bix/Core/Timer.h"
#include "Bix/Core/Window.h"
#include "Bix/Game/SceneSerializer.h"
#include "Bix/Graphics/Renderer.h"
#include "Bix/Input/Input.h"
#include "Bix/Engine/Gui/GuiManager.h"

namespace BixEngine::Game
{
    namespace
    {
        constexpr const char* kSceneModule = "Game";
        constexpr ::BixEngine::Game::Scripting::ScriptMetadataEntry kSceneMetadata[] =
        {
            {"IncludePath", "Bix/Game/Scene.h"},
        };
    }

    BIX_DEFINE_SCRIPT_CLASS(Scene, (::BixEngine::Game::Scripting::ScriptRegistrationDescriptor{
        .name = "Scene",
        .moduleName = kSceneModule,
        .kind = ::BixEngine::Game::Scripting::ScriptKind::Scene,
        .isAbstract = true,
        .category = "World",
        .tooltip = "Abstract scene container for actors.",
        .keywords = "Scene,World,Level",
        .metadata = kSceneMetadata,
        .metadataCount = std::size(kSceneMetadata),
    }));

    Scene::Scene(String name) : name_(std::move(name)) {}

    void Scene::SetContext(SceneContext context) noexcept
    {
        context_ = context;
    }

    void Scene::AddActor(std::unique_ptr<Actor> actor)
    {
        if (actor)
            SceneSerializer::EnsureActorFactory(*actor);
        actors_.push_back(std::move(actor));
    }

    void Scene::ClearActors() noexcept
    {
        actors_.clear();
    }

    void Scene::Rename(String name)
    {
        name_ = std::move(name);
    }

    void Scene::SetName(String name)
    {
        name_ = std::move(name);
    }

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

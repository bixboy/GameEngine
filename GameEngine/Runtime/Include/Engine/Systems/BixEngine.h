#pragma once
#include "Engine/Systems/Application.h"
#include "Core/Logger.h"
#include "Core/Timer.h"
#include "Engine/Systems/Window.h"

#include "Graphics/Renderer.h"

#include "Input/Input.h"
#include "Input/InputManager.h"

#include "Game/Actor.h"
#include "Game/EmptyScene.h"
#include "Game/Scene.h"
#include "Game/SceneManager.h"
#include "Game/Test/TestScene.h"

#include "Core/Math/Math.h"

namespace BixEngine
{
    using Application = Core::Application;
    using Window = Core::Window;
    using Timer = Core::Timer;

    using Renderer = Graphics::Renderer;

    using InputSystem = Input::Input;
    using InputManager = Input::InputManager;
    using InputEvent = Input::InputEvent;

    using Scene = Game::Scene;
    using SceneManager = Game::SceneManager;
    using Actor = Game::Actor;
    using EmptyScene = Game::EmptyScene;
    using TestScene = Game::TestScene;
}

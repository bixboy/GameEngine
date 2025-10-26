#pragma once
#include "Bix/Core/Application.h"
#include "Bix/Core/Logger.h"
#include "Bix/Core/Timer.h"
#include "Bix/Core/Window.h"

#include "Bix/Graphics/Renderer.h"

#include "Bix/Input/Input.h"
#include "Bix/Input/InputManager.h"

#include "Bix/Game/Actor.h"
#include "Bix/Game/EmptyScene.h"
#include "Bix/Game/Scene.h"
#include "Bix/Game/SceneManager.h"
#include "Bix/Game/Test/TestScene.h"

#include "Bix/Math/Math.h"

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

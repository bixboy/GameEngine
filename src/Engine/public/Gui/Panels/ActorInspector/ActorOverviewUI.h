#pragma once
#include "Gui/Panels/ActorInspector/ActorInspectorState.h"

namespace BixEngine::Game
{
    class Actor;
}

namespace BixEngine::Gui::ActorInspector
{
    void DrawActorOverview(Game::Actor& actor, ActorInspectorState& state);
    void DrawGeneralSection(Game::Actor& actor);
}

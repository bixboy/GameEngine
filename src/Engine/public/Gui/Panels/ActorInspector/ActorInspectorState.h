#pragma once
#include <array>
#include "Actor.h"

namespace BixEngine::Gui::ActorInspector
{
    struct ActorInspectorState
    {
        std::array<char, 256> nameBuffer {};
    };

    ActorInspectorState& GetActorState(const Game::Actor& actor);
}

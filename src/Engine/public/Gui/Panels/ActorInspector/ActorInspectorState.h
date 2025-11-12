#pragma once
#include <array>


namespace BixEngine
{
    class String;

    namespace Game
    {
        class Actor;
    }
}

namespace BixEngine::Gui::ActorInspector
{
    struct ActorInspectorState
    {
        std::array<char, 256> nameBuffer{};
    };

    ActorInspectorState& GetActorState(const Game::Actor& actor);
}

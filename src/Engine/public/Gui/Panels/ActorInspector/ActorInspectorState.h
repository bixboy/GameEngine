#pragma once
#include <array>
#include <string>
#include "Framework/Actor.h"


namespace BixEngine::Gui::ActorInspector
{
    struct ActorInspectorState
    {
        std::array<char, 256> nameBuffer{};
        std::string cachedName{}; 
    };

    ActorInspectorState& GetActorState(const Game::Actor& actor);
    
    void ClearActorInspectorCache();
}
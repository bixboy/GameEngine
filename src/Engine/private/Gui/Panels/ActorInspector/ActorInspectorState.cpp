#include "Gui/Panels/ActorInspector/ActorInspectorState.h"
#include "Gui/Panels/ActorInspector/Utils/ActorInspectorHelpers.h"
#include "Framework/Actor.h"
#include <unordered_map>
#include <algorithm>


namespace BixEngine::Gui::ActorInspector
{
    namespace
    {
        using StateMap = std::unordered_map<std::string, ActorInspectorState>;

        StateMap& GetActorStates()
        {
            static StateMap states;
            return states;
        }
        
        void SynchronizeNameBuffer(const Game::Actor& actor, ActorInspectorState& state)
        {
            const auto nameView = actor.GetName().View();
            const std::size_t maxCopy = state.nameBuffer.size() - 1;
            const std::size_t copyLength = std::min<std::size_t>(nameView.size(), maxCopy);

            if (std::strncmp(state.nameBuffer.data(), nameView.data(), copyLength) != 0 ||
                state.nameBuffer[copyLength] != '\0')
            {
                std::memcpy(state.nameBuffer.data(), nameView.data(), copyLength);
                state.nameBuffer[copyLength] = '\0';
            }
        }
    }

    
    ActorInspectorState& GetActorState(const Game::Actor& actor)
    {
        auto& states = GetActorStates();
        const std::string key = BuildActorStateKey(actor);
        auto [it, inserted] = states.try_emplace(key);

        ActorInspectorState& state = it->second;
        if (inserted)
        {
            state.nameBuffer.fill('\0');
        }

        SynchronizeNameBuffer(actor, state);
        return state;
    }
}

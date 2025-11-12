#pragma once

#include "Core/Containers/String.h"
#include "Game/Actor.h"
#include "Game/Scene.h"

#include <algorithm>
#include <string>

namespace BixEngine::Gui::ActorInspector
{
    [[nodiscard]] inline std::string ToStdString(const BixEngine::String& value)
    {
        const auto view = value.View();
        return std::string(view.data(), view.size());
    }

    [[nodiscard]] inline std::string BuildActorContextId(const Game::Actor& actor)
    {
        const auto nameView = actor.GetName().View();
        return std::string(nameView.data(), nameView.size());
    }

    [[nodiscard]] inline std::string BuildActorStateKey(const Game::Actor& actor)
    {
        const auto uuidView = actor.GetUUID().View();
        return std::string(uuidView.data(), uuidView.size());
    }

    [[nodiscard]] inline bool ActorBelongsToScene(const Game::Scene& scene, const Game::Actor* actor)
    {
        if (!actor)
        {
            return false;
        }

        const auto& actors = scene.GetActors();
        return std::any_of(actors.begin(), actors.end(), [actor](const auto& candidate)
        {
            return candidate.get() == actor;
        });
    }
}


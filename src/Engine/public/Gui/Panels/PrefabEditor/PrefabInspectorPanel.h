#pragma once

#include "Gui/Panels/ActorInspectorPanel.h"

namespace BixEngine::Gui
{
    class PrefabInspectorPanel final : public ActorInspectorPanel
    {
    public:
        PrefabInspectorPanel(
            std::function<Game::Scene*()> sceneProvider,
            std::function<Game::Actor*()> selectionGetter,
            std::function<void(Game::Actor*)> selectionSetter
        );

        ~PrefabInspectorPanel() override = default;
    };
}

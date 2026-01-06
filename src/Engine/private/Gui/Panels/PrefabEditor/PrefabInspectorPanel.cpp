#include "Gui/Panels/PrefabEditor/PrefabInspectorPanel.h"

namespace BixEngine::Gui
{
    PrefabInspectorPanel::PrefabInspectorPanel(std::function<Game::Scene*()> sceneProvider, std::function<Game::Actor*()> selectionGetter, std::function<void(Game::Actor*)> selectionSetter)
        : ActorInspectorPanel(std::move(sceneProvider), std::move(selectionGetter), std::move(selectionSetter))
    {
    }
}

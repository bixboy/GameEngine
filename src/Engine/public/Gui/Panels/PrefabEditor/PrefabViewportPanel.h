#pragma once

#include "Gui/Panels/SceneViewportPanel.h"
#include <functional>

namespace BixEngine::Game
{
    class Scene;
    class Actor;
}

namespace BixEngine::Gui
{
    class PrefabViewportPanel final : public SceneViewportPanel
    {
    public:
        PrefabViewportPanel(
            std::function<SDL_Texture*()> textureProvider,
            std::function<std::pair<int, int>()> sizeProvider,
            std::function<Game::Scene*()> sceneProvider,
            std::function<Game::Actor*()> selectionGetter,
            std::function<void(Game::Actor*)> selectionSetter
        );

        ~PrefabViewportPanel() override = default;
    };
}

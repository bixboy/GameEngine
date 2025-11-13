#pragma once

namespace BixEngine::Game
{
    class Actor;
}

namespace BixEngine::Gui::ActorInspector
{
    void DrawComponentSection(Game::Actor& actor);
    void DrawAddComponentPopup(Game::Actor& actor);
}

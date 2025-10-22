#include "Bix/Game/Components/Component.h"

namespace BixEngine::Game
{
    BIX_DEFINE_SCRIPT_CLASS(Component, (::BixEngine::Game::Scripting::ScriptRegistrationDescriptor{
        .name = "Component",
        .moduleName = "Game",
        .kind = ::BixEngine::Game::Scripting::ScriptKind::Component,
        .isAbstract = true,
    }));
}


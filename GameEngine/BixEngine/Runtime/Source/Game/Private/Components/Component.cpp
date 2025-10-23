#include "Bix/Game/Components/Component.h"

namespace BixEngine::Game
{
    namespace
    {
        constexpr const char* kComponentModule = "Game";
    }

    BIX_DEFINE_SCRIPT_CLASS(Component, (::BixEngine::Game::Scripting::ScriptRegistrationDescriptor{
        .name = "Component",
        .moduleName = kComponentModule,
        .kind = ::BixEngine::Game::Scripting::ScriptKind::Component,
        .isAbstract = true,
    }));

    BIX_IMPLEMENT_CLASS(Component);

    void Component::RegisterProperties(::BixEngine::Engine::SaveSystem::BixClass& cls)
    {
        Object::RegisterProperties(cls);
    }
}


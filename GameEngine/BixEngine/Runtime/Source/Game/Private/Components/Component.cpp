#include "Bix/Game/Components/Component.h"

namespace BixEngine::Game
{ 
    BIX_IMPLEMENT_CLASS(Component);

    void Component::RegisterProperties(::BixEngine::Engine::SaveSystem::BixClass& cls)
    {
        Object::RegisterProperties(cls);
    }
}


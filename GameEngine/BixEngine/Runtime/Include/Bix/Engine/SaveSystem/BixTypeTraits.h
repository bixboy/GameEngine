#pragma once

#include <type_traits>

namespace BixEngine::Engine::SaveSystem
{
    template<typename T>
    struct AlwaysFalse : std::false_type {};
}

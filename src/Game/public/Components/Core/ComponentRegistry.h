#pragma once

namespace BixEngine::Game
{
    /**
     * Ensures that all built-in engine components are registered with the
     * reflection system so they can be instantiated dynamically at runtime.
     */
    void RegisterBuiltinComponents();
}

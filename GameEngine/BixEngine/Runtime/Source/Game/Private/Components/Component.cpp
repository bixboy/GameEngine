#include "Bix/Game/Components/Component.h"

#include <array>

namespace BixEngine::Game
{
    namespace
    {
        constexpr ::BixEngine::Game::Scripting::ScriptMetadataEntry kComponentMetadata[] =
        {
            {"IncludePath", "Bix/Game/Components/Component.h"},
            {"EditorIcon", "Icons/Component"},
        };
    }

    BIX_DEFINE_SCRIPT_CLASS(Component, (::BixEngine::Game::Scripting::ScriptRegistrationDescriptor{
        .name = "Component",
        .moduleName = "Game",
        .kind = ::BixEngine::Game::Scripting::ScriptKind::Component,
        .isAbstract = true,
        .category = "Gameplay",
        .tooltip = "Base gameplay component class.",
        .keywords = "Component,Gameplay",
        .metadata = kComponentMetadata,
        .metadataCount = std::size(kComponentMetadata),
    }));
}


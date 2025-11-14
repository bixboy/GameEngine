#pragma once

#include "Gui/Panels/ActorInspector/ActorInspectorSection.h"

namespace BixEngine::Gui::ActorInspector
{
    class TransformInspectorSection final : public ActorInspectorSection
    {
    public:
        void Draw(Game::Actor& actor) override;
    };
}

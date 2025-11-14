#pragma once
#include "Gui/Panels/ActorInspector/InspectorSections/ActorInspectorSection.h"

namespace BixEngine::Gui::ActorInspector
{
    class ComponentInspectorSection final : public ActorInspectorSection
    {
    public:
        void Draw(Game::Actor& actor) override;

    private:
        void DrawAddComponentPopup(Game::Actor& actor);
    };
}

#pragma once
#include "Gui/Panels/ActorInspector/InspectorSections/ActorInspectorSection.h"


namespace BixEngine::Gui::ActorInspector
{
    class GeneralInspectorSection final : public ActorInspectorSection
    {
    public:
        void Draw(Game::Actor& actor) override;
    };
}

#pragma once
#include "Gui/DefaultEngineGui.h"
#include "Gui/Internal/GuiPanel.h"


namespace BixEngine::Gui
{
    GuiPanel& CreateActorInspectorPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context);
}

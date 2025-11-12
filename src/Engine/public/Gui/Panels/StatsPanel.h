#pragma once
#include "Gui/DefaultEngineGui.h"
#include "Gui/Internal/GuiPanel.h"


namespace BixEngine::Gui
{
    GuiPanel& CreateStatsPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context);
}

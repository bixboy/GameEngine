#pragma once

#include "Engine/Gui/DefaultEngineGui.h"
#include "Engine/Gui/Internal/GuiPanel.h"

namespace BixEngine::Gui
{
    GuiPanel& CreateStatsPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context);
}

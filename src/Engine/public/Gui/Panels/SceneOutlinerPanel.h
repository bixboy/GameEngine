#pragma once

#include "Gui/DefaultEngineGui.h"
#include "Gui/Internal/GuiPanel.h"

namespace BixEngine::Gui
{
    GuiPanel& CreateSceneOutlinerPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context);
}

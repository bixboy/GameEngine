#pragma once
#include "Gui/DefaultEngineGui.h"
#include "Gui/Internal/GuiPanel.h"

namespace BixEngine::Gui
{
    GuiPanel& CreateSceneViewportPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context);
}

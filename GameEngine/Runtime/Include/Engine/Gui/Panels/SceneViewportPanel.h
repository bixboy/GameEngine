#pragma once

#include "Engine/Gui/DefaultEngineGui.h"
#include "Engine/Gui/Core/GuiPanel.h"

namespace BixEngine::Gui
{
    GuiPanel& CreateSceneViewportPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context);
}


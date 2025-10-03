#include "Gui/Panels/StatsPanel.h"

#include "Core/Timer.h"
#include "Game/Scene.h"
#include "Game/SceneManager.h"
#include "Gui/GuiManager.h"
#include "Gui/GuiPanel.h"


#include "imgui.h"

namespace Engine::Gui
{
    namespace
    {
        constexpr ImVec4 kStatsBackground{0.1f, 0.1f, 0.1f, 0.95f};
    }

    GuiPanel& CreateStatsPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context)
    {
        GuiPanel& statsPanel = guiManager.CreatePanel("engine_stats", "Engine Stats");
        statsPanel.SetPosition(0.f, 50.0f);
        statsPanel.SetSize(300.0f, 200.0f);
        statsPanel.SetResizable(false);
        statsPanel.SetMovable(true);
        statsPanel.SetCollapsable(true);
        statsPanel.SetClosable(true);
        statsPanel.SetBackgroundColor(kStatsBackground);
        statsPanel.AddWindowFlags(ImGuiWindowFlags_NoCollapse);
        statsPanel.SetDrawFunction([timer = context.timer,
                                    lastDeltaTime = context.lastDeltaTime,
                                    provider = context.sceneManagerProvider]()
        {
            const float fps = timer ? timer->GetFPS() : 0.0f;
            const float deltaMs = lastDeltaTime ? (*lastDeltaTime * 1000.0f) : 0.0f;

            ImGui::Text("FPS: %.1f", fps);
            ImGui::Text("Delta Time: %.3f ms", deltaMs);

            Game::SceneManager* sceneManager = provider ? provider() : nullptr;
            if (!sceneManager)
                return;

            if (const auto* activeScene = sceneManager->GetScene())
            {
                const String& sceneName = activeScene->Name();
                const auto sceneNameView = sceneName.View();
                ImGui::Separator();
                ImGui::Text(
                    "Scene: %.*s",
                    static_cast<int>(sceneNameView.size()),
                    sceneNameView.data());
            }
        });

        return statsPanel;
    }
}

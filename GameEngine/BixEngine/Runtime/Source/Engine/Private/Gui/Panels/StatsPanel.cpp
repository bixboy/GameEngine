#include "Gui/Panels/StatsPanel.h"

#include "Core/Timer.h"
#include "Game/Scene.h"
#include "Game/SceneManager.h"
#include "Gui/GuiManager.h"
#include "Gui/GuiPanel.h"

#include "imgui.h"
#include <cmath>
#include <numeric>

namespace BixEngine::Gui
{
    namespace
    {
        constexpr ImVec4 kStatsBackground{0.1f, 0.1f, 0.1f, 0.95f};
        constexpr float kSmoothingFactor = 0.1f;
        constexpr float kUpdateInterval  = 0.25f;
    }

    GuiPanel& CreateStatsPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context)
    {
        GuiPanel& statsPanel = guiManager.CreatePanel("engine_stats", "Engine Stats");
        guiManager.SetPanelDockingArea(statsPanel, DockSpaceRegion::Right);
        statsPanel.SetResizable(false);
        statsPanel.SetMovable(true);
        statsPanel.SetCollapsable(true);
        statsPanel.SetClosable(true);
        statsPanel.SetBackgroundColor(kStatsBackground);
        statsPanel.AddWindowFlags(ImGuiWindowFlags_NoCollapse);

        statsPanel.SetDrawFunction([timer = context.timer, lastDeltaTime = context.lastDeltaTime, provider = context.sceneManagerProvider]()
        {
            const float fps      = timer ? timer->GetFPS() : 0.0f;
            const float deltaMs  = lastDeltaTime ? (*lastDeltaTime * 1000.0f) : 0.0f;

            static float smoothedFps     = 0.0f;
            static float smoothedDelta   = 0.0f;
            static float displayedFps    = 0.0f;
            static float displayedDelta  = 0.0f;
            static float timeSinceUpdate = 0.0f;

            if (!std::isnan(fps) && fps > 0.0f)
                smoothedFps = smoothedFps * (1.0f - kSmoothingFactor) + fps * kSmoothingFactor;

            if (!std::isnan(deltaMs) && deltaMs > 0.0f)
                smoothedDelta = smoothedDelta * (1.0f - kSmoothingFactor) + deltaMs * kSmoothingFactor;

            if (lastDeltaTime)
                timeSinceUpdate += *lastDeltaTime;

            if (timeSinceUpdate >= kUpdateInterval)
            {
                displayedFps   = smoothedFps;
                displayedDelta = smoothedDelta;
                timeSinceUpdate = 0.0f;
            }

            // FPS Color
            ImVec4 fpsColor;
            if (displayedFps < 30.0f)
                fpsColor = {1.0f, 0.2f, 0.2f, 1.0f}; // rouge
            else if (displayedFps < 60.0f)
                fpsColor = {1.0f, 1.0f, 0.2f, 1.0f}; // jaune
            else
                fpsColor = {0.2f, 1.0f, 0.2f, 1.0f}; // vert

            ImGui::TextColored(fpsColor, "FPS: %.1f", displayedFps);
            ImGui::Text("Frame Time: %.2f ms", displayedDelta);
            ImGui::Separator();
            ImGui::Text("Instant FPS: %.1f | ms: %.2f", fps, deltaMs);

            // --- Scene Infos ---
            Game::SceneManager* sceneManager = provider ? provider() : nullptr;
            if (!sceneManager)
                return;

            if (const auto* activeScene = sceneManager->GetScene())
            {
                const String& sceneName = activeScene->Name();
                const auto sceneNameView = sceneName.View();

                ImGui::Separator();
                ImGui::Text("Scene: %.*s",
                    static_cast<int>(sceneNameView.size()),
                    sceneNameView.data());
            }
        });

        return statsPanel;
    }
}

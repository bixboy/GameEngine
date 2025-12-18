#include "Gui/Panels/StatsPanel.h"

#include <cmath>
#include <string>
#include <vector>

#include "Time/Timer.h"
#include "Framework/Scene.h"
#include "Framework/SceneManager.h"
#include "Gui/Widgets/Widgets.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Input.h"
#include "Gui/Core/EditorPreferences.h"

#include "imgui.h"

namespace BixEngine::Gui
{
    using namespace Utils;

    namespace
    {
        std::string FormatValue(float value, const char* format)
        {
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), format, value);
            return std::string(buffer);
        }
    }

    StatsPanel::StatsPanel(Core::Timer* timer, const float* lastDeltaTime, std::function<Game::SceneManager*()> sceneProvider, std::function<const Input::MouseStatistics*()> mouseStatsProvider)
        : GuiPanelBase("engine_stats"),
        timer_(timer),
        lastDeltaTime_(lastDeltaTime),
        sceneManagerProvider_(std::move(sceneProvider)),
        mouseStatsProvider_(std::move(mouseStatsProvider))
    {
    }

    StatsPanel::StatsPanel(const DefaultEngineGuiContext& context) : StatsPanel(context.timer, context.lastDeltaTime, context.sceneManagerProvider, context.mouseStatsProvider)
    {
    }

    void StatsPanel::Draw()
    {
        const float fps = timer_ ? timer_->GetFPS() : 0.0f;
        const float deltaMs = lastDeltaTime_ ? (*lastDeltaTime_ * 1000.0f) : 0.0f;
        const ImGuiIO& io = ImGui::GetIO();

        const auto& settings = EditorSettings::Get();
        const float smoothing = settings.StatsSmoothingFactor;
        const float updateInt = settings.StatsUpdateInterval;

        if (!std::isnan(fps) && fps > 0.0f)
            smoothedFps_ = smoothedFps_ * (1.0f - smoothing) + fps * smoothing;

        if (!std::isnan(deltaMs) && deltaMs > 0.0f)
            smoothedDelta_ = smoothedDelta_ * (1.0f - smoothing) + deltaMs * smoothing;

        if (lastDeltaTime_)
            timeSinceUpdate_ += *lastDeltaTime_;

        if (timeSinceUpdate_ >= updateInt)
        {
            displayedFps_ = smoothedFps_;
            displayedDelta_ = smoothedDelta_;
            timeSinceUpdate_ = 0.0f;
        }

        Widgets::DrawPanelHeader({
            .title = "Performance",
            .subtitle = "Real-time engine statistics",
            .showSeparator = false
        });

        std::vector<Widgets::MetricDisplay> metrics;
        metrics.reserve(4);
        
        metrics.emplace_back(
            "Average FPS",
            FormatValue(displayedFps_, "%.1f"),
            Widgets::Metrics::ColorForFps(displayedFps_),
            "Smoothed framerate");
        
        metrics.emplace_back(
            "Average Frame Time",
            FormatValue(displayedDelta_, "%.2f ms"),
            ImVec4{0.8f, 0.8f, 1.0f, 1.0f},
            "Smoothed frame time");
        
        metrics.emplace_back(
            "Instant FPS",
            FormatValue(fps, "%.1f"),
            Widgets::Metrics::ColorForFps(fps),
            "Current frame");
        
        metrics.emplace_back(
            "Instant Frame Time",
            FormatValue(deltaMs, "%.2f ms"),
            ImVec4{0.6f, 0.9f, 1.0f, 1.0f},
            "Current frame");

        Widgets::DrawMetricsTable(metrics, 160.0f);

        Widgets::PanelSection sceneSection("Scene Information");
        if (sceneSection.IsOpen())
        {
            Game::SceneManager* sceneManager = sceneManagerProvider_ ? sceneManagerProvider_() : nullptr;
            if (!sceneManager)
            {
                DrawEmptyStateMessage("No active scene.");
            }
            else if (const Game::Scene* activeScene = sceneManager->GetScene())
            {
                const String& sceneName = activeScene->GetName();
                const auto view = sceneName.View();
                ImGui::Text("Scene: %.*s", static_cast<int>(view.size()), view.data());
                ImGui::Text("Actors: %zu", activeScene->GetActors().size());
            }
            else
            {
                DrawEmptyStateMessage("No scene is currently loaded.");
            }
        }

        Widgets::PanelSection inputSection("Input Statistics");
        if (inputSection.IsOpen())
        {
            ImGui::Text("Mouse delta: %.1f, %.1f", io.MouseDelta.x, io.MouseDelta.y);

            const Input::MouseStatistics* stats = mouseStatsProvider_ ? mouseStatsProvider_() : nullptr;
            if (!stats)
            {
                DrawEmptyStateMessage("Mouse statistics unavailable.");
            }
            else
            {
                ImGui::Text("Mouse events/s: %d", stats->eventsPerSecond);
                ImGui::Text("Dropped events/s: %d", stats->droppedEventsPerSecond);
                ImGui::Text("Mouse events/frame: %d", stats->processedEventsThisFrame);
                ImGui::Text("Dropped/frame: %d", stats->droppedEventsThisFrame);
            }
        }
    }
}

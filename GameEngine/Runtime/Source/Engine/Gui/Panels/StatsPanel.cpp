#include "Engine/Gui/Panels/StatsPanel.h"

#include "Core/Timer.h"
#include "Game/Scene.h"
#include "Game/SceneManager.h"
#include "Engine/Gui/Core/GuiManager.h"
#include "Engine/Gui/Core/GuiPanel.h"
#include "Engine/Gui/Utils/GuiPanelController.h"
#include "Engine/Gui/Widgets/GuiWidgetLibrary.h"
#include "Engine/Gui/Utils/GuiHelpers.h"

#include "imgui.h"
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#include "Engine/Gui/Core/GuiDocking.h"

namespace BixEngine::Gui
{
    namespace
    {
        constexpr ImVec4 kStatsBackground{0.1f, 0.1f, 0.1f, 0.95f};
        constexpr float kSmoothingFactor = 0.1f;
        constexpr float kUpdateInterval  = 0.25f;

        ImVec4 ComputeFpsColor(float fps) noexcept
        {
            if (fps < 30.0f)
                return {1.0f, 0.2f, 0.2f, 1.0f};
            
            if (fps < 60.0f)
                return {1.0f, 1.0f, 0.2f, 1.0f};
            
            return {0.2f, 1.0f, 0.2f, 1.0f};
        }

        std::string FormatValue(float value, const char* format)
        {
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), format, value);
            
            return std::string(buffer);
        }

        class StatsPanelController final : public GuiPanelController
        {
        public:
            StatsPanelController(Core::Timer* timer,
                                 const float* lastDeltaTime,
                                 std::function<Game::SceneManager*()> sceneProvider)
                : timer_(timer)
                , lastDeltaTime_(lastDeltaTime)
                , sceneManagerProvider_(std::move(sceneProvider))
            {
            }

        protected:
            void OnAttach(GuiPanel& panel) override
            {
                panel.SetResizable(false);
                panel.SetMovable(true);
                panel.SetCollapsable(true);
                panel.SetClosable(true);
                panel.SetBackgroundColor(kStatsBackground);
                panel.AddWindowFlags(ImGuiWindowFlags_NoCollapse);
            }

            void OnDraw(GuiPanel&) override
            {
                const float fps = timer_ ? timer_->GetFPS() : 0.0f;
                const float deltaMs = lastDeltaTime_ ? (*lastDeltaTime_ * 1000.0f) : 0.0f;

                if (!std::isnan(fps) && fps > 0.0f)
                    smoothedFps_ = smoothedFps_ * (1.0f - kSmoothingFactor) + fps * kSmoothingFactor;

                if (!std::isnan(deltaMs) && deltaMs > 0.0f)
                    smoothedDelta_ = smoothedDelta_ * (1.0f - kSmoothingFactor) + deltaMs * kSmoothingFactor;

                if (lastDeltaTime_)
                    timeSinceUpdate_ += *lastDeltaTime_;

                if (timeSinceUpdate_ >= kUpdateInterval)
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
                metrics.push_back({"Average FPS", FormatValue(displayedFps_, "%.1f"), ComputeFpsColor(displayedFps_), "Smoothed framerate"});
                metrics.push_back({"Average Frame Time", FormatValue(displayedDelta_, "%.2f ms"), ImVec4{0.8f, 0.8f, 1.0f, 1.0f}, "Smoothed frame time"});
                metrics.push_back({"Instant FPS", FormatValue(fps, "%.1f"), ComputeFpsColor(fps), "Current frame"});
                metrics.push_back({"Instant Frame Time", FormatValue(deltaMs, "%.2f ms"), ImVec4{0.6f, 0.9f, 1.0f, 1.0f}, "Current frame"});

                Widgets::DrawMetricsTable(metrics, 160.0f);

                Widgets::PanelSection sceneSection("Scene Information");
                if (sceneSection.IsOpen())
                {
                    Game::SceneManager* sceneManager = sceneManagerProvider_ ? sceneManagerProvider_() : nullptr;
                    if (!sceneManager)
                    {
                        Utils::DrawEmptyStateMessage("No active scene.");
                        return;
                    }

                    if (const Game::Scene* activeScene = sceneManager->GetScene())
                    {
                        const String& sceneName = activeScene->GetName();
                        const auto view = sceneName.View();
                        ImGui::Text("Scene: %.*s", static_cast<int>(view.size()), view.data());
                        ImGui::Text("Actors: %zu", activeScene->GetActors().size());
                    }
                    else
                    {
                        Utils::DrawEmptyStateMessage("No scene is currently loaded.");
                    }
                }
            }

        private:
            Core::Timer* timer_{nullptr};
            const float* lastDeltaTime_{nullptr};
            std::function<Game::SceneManager*()> sceneManagerProvider_{};

            float smoothedFps_{0.0f};
            float smoothedDelta_{0.0f};
            float displayedFps_{0.0f};
            float displayedDelta_{0.0f};
            float timeSinceUpdate_{0.0f};
        };
    }

    GuiPanel& CreateStatsPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context)
    {
        auto registration = guiManager.RegisterUtilityPanel<StatsPanelController>(
            "engine_stats",
            "Engine Stats",
            context.timer,
            context.lastDeltaTime,
            context.sceneManagerProvider);

        guiManager.SetPanelDockingArea(registration.panel, DockSpaceRegion::Right);
        return registration.panel;
    }
}

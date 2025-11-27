#pragma once
#include <functional>

#include "Gui/Core/DefaultEngineGui.h"
#include "Gui/Base/GuiPanelBase.h"


namespace BixEngine::Gui
{
    class StatsPanel : public GuiPanelBase
    {
    public:
        StatsPanel(Core::Timer* timer, const float* lastDeltaTime, std::function<Game::SceneManager*()> sceneProvider, std::function<const Input::MouseStatistics*()> mouseStatsProvider);
        explicit StatsPanel(const DefaultEngineGuiContext& context);

        void Draw() override;

    private:
        Core::Timer* timer_{nullptr};
        const float* lastDeltaTime_{nullptr};
        std::function<Game::SceneManager*()> sceneManagerProvider_{};
        std::function<const Input::MouseStatistics*()> mouseStatsProvider_{};

        float smoothedFps_{0.0f};
        float smoothedDelta_{0.0f};
        float displayedFps_{0.0f};
        float displayedDelta_{0.0f};
        float timeSinceUpdate_{0.0f};
    };
}

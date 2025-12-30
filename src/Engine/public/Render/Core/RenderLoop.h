#pragma once
#include "Math/Color.h"

namespace BixEngine
{
    namespace Graphics { class Renderer; }
    namespace Core
    {
        class SubsystemManager;
        class GuiModule;
    }
}

namespace BixEngine::Core
{
    class RenderLoop
    {
    public:
        RenderLoop() = default;
        ~RenderLoop() = default;

        void Configure(SubsystemManager* subsystems, GuiModule* guiModule, Graphics::Renderer* renderer, Math::Color clearColor) noexcept;

        void Reset() noexcept;

        // --- Runtime ---

        float CalculateDeltaTime();
        
        void BeginFrame();

        void Update(float deltaTime);

        void Render();

        // --- Getters / Setters ---

        void SetClearColor(const Math::Color& color) noexcept { clearColor_ = color; }
        [[nodiscard]] Math::Color GetClearColor() const noexcept { return clearColor_; }

        [[nodiscard]] const float* GetLastDeltaTimePointer() const noexcept { return &lastDeltaTime_; }

    private:
        SubsystemManager* subsystems_{nullptr};
        GuiModule* guiModule_{nullptr};
        Graphics::Renderer* renderer_{nullptr};

        Math::Color clearColor_{0, 0, 0, 255};
        float lastDeltaTime_{0.0f};
    };
}
#pragma once

#include <filesystem>
#include <functional>
#include <memory>

#include "Core/Containers/String.h"
#include "Engine/Gui/Controllers/GuiPanelController.h"

struct ImVec2;

namespace BixEngine
{
    namespace Core { class SubsystemManager; }
    namespace Game { class Actor; }
}

namespace BixEngine::Gui
{
    class ActorEditorController final : public GuiPanelController
    {
    public:
        enum class Section
        {
            Toolbar,
            Viewport,
            Outline,
            Inspector
        };

        using CloseRequest = std::function<void()>;

        struct SharedState
        {
            Core::SubsystemManager* subsystems{nullptr};
            std::filesystem::path assetPath{};
            String assetDisplayName{};
            String stableIdRoot{};
            CloseRequest onCloseRequest{};
            Game::Actor* actor{nullptr};
            bool actorRefreshRequested{true};
        };

        ActorEditorController(std::shared_ptr<SharedState> sharedState, Section section);

        static std::shared_ptr<SharedState> CreateSharedState(Core::SubsystemManager& subsystems, std::filesystem::path assetPath, String stableIdRoot, CloseRequest onCloseRequest);

        [[nodiscard]] const std::filesystem::path& GetAssetPath() const noexcept { return state_->assetPath; }
        [[nodiscard]] const String& GetDisplayName() const noexcept { return state_->assetDisplayName; }
        [[nodiscard]] std::shared_ptr<SharedState> GetSharedState() const noexcept { return state_; }

        void RequestActorReload();

    protected:
        void OnAttach(GuiPanel& panel) override;
        void OnDetach(GuiPanel& panel) override;
        void OnDraw(GuiPanel& panel) override;

    private:
        void EnsureActorUpToDate();
        void DrawToolbar();
        void DrawViewport();
        void DrawOutline();
        void DrawInspector();

        void ApplyPanelTitle(GuiPanel& panel);

        void HandlePlayRequest();
        void HandleSaveRequest();
        void HandleCompileRequest();

        void DrawViewportGrid_(const ImVec2& size);

        Section section_;
        std::shared_ptr<SharedState> state_;
        String cachedDisplayName_{};
        String stableId_{};
    };
}

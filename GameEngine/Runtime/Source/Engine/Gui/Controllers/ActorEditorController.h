#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>

#include "Core/Containers/String.h"
#include "Engine/Gui/Utils/GuiPanelController.h"

struct ImVec2;

namespace BixEngine
{
    namespace Core { class SubsystemManager; }
    namespace Game { class Actor; }
}

namespace BixEngine::Gui
{
    class ActorEditorPanel;

    class ActorEditorController final : public GuiPanelController
    {
    public:
        using CloseRequest = std::function<void()>;

        ActorEditorController(Core::SubsystemManager& subsystems,
                              std::filesystem::path assetPath,
                              CloseRequest onCloseRequest);

        [[nodiscard]] const std::filesystem::path& GetAssetPath() const noexcept { return assetPath_; }
        [[nodiscard]] const String& GetDisplayName() const noexcept { return assetDisplayName_; }

        void RequestActorReload();

    protected:
        void OnAttach(GuiPanel& panel) override;
        void OnDetach(GuiPanel& panel) override;
        void OnDraw(GuiPanel& panel) override;

    private:
        [[nodiscard]] ActorEditorPanel* GetActorPanel(GuiPanel& panel) noexcept;
        [[nodiscard]] Game::Actor* ResolveActor() noexcept;
        void EnsureActorUpToDate();

        void DrawViewport();
        void DrawOutline();
        void DrawInspector();

        void HandlePlayRequest();
        void HandleSaveRequest();
        void HandleCompileRequest();

        void DrawViewportGrid_(const ImVec2& size);

        Core::SubsystemManager* subsystems_{nullptr};
        std::filesystem::path assetPath_{};
        String assetDisplayName_{};
        CloseRequest onCloseRequest_{};
        Game::Actor* actor_{nullptr};
        bool actorRefreshRequested_{true};
    };
}

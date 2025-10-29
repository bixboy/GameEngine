#pragma once

#include <array>
#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Core/Containers/String.h"
#include "Engine/Gui/Controllers/ActorEditorController.h"
#include "Engine/Gui/Core/GuiLayoutManager.h"

namespace BixEngine::Gui
{
    class GuiManager;
    class GuiPanel;
}

namespace BixEngine::Core
{
    class SubsystemManager;
    
    class GuiActorEditorManager
    {
    public:
        struct ActorEditorPanels
        {
            Gui::GuiPanel* toolbar{nullptr};
            Gui::GuiPanel* viewport{nullptr};
            Gui::GuiPanel* outline{nullptr};
            Gui::GuiPanel* inspector{nullptr};

            [[nodiscard]] std::size_t Count() const noexcept
            {
                std::size_t count = 0;
                if (toolbar)   ++count;
                if (viewport)  ++count;
                if (outline)   ++count;
                if (inspector) ++count;
                return count;
            }

            template <typename Fn>
            void ForEachPanel(Fn&& fn) const
            {
                if (toolbar)   std::forward<Fn>(fn)(toolbar);
                if (viewport)  std::forward<Fn>(fn)(viewport);
                if (outline)   std::forward<Fn>(fn)(outline);
                if (inspector) std::forward<Fn>(fn)(inspector);
            }

            [[nodiscard]] std::span<Gui::GuiPanel*> CopyTo(std::span<Gui::GuiPanel*> buffer) const noexcept
            {
                std::size_t index = 0;
                auto push = [&](Gui::GuiPanel* panel) noexcept
                {
                    if (!panel || index >= buffer.size())
                        return;
                    buffer[index++] = panel;
                };

                push(toolbar);
                push(viewport);
                push(outline);
                push(inspector);

                return buffer.first(index);
            }
        };

        struct ActorEditorEntry
        {
            std::filesystem::path assetPath;
            std::string navigationId;
            std::string buttonLabel;
            ActorEditorPanels panels{};
            std::shared_ptr<Gui::ActorEditorController::SharedState> sharedState{};
        };
        
        using FocusRequestCallback = std::function<void(const std::string&)>;
        using FocusSceneCallback   = std::function<void()>;
        
        GuiActorEditorManager(Gui::GuiManager& guiManager, Gui::GuiLayoutManager* layoutManager, FocusRequestCallback focusRequestCallback, FocusSceneCallback focusSceneCallback);

        void SetLayoutManager(Gui::GuiLayoutManager* layoutManager) noexcept;
        void SetSubsystems(SubsystemManager* subsystems) noexcept;
        void SetFocusCallbacks(FocusRequestCallback focusRequestCallback, FocusSceneCallback focusSceneCallback);
        
        void OpenActorEditor(const std::filesystem::path& path);
        void CloseActorEditor(const std::string& navigationId);

        void ActivateEditor(std::string_view navigationId, bool requestFocus);
        void ActivateScene(bool requestFocus);

        void RefreshActorPanelsVisibility();
        void RemoveAllEditors();

        void OnLayoutChanged(Gui::EditorLayoutType layout) noexcept;
        
        [[nodiscard]] bool HasEditors() const noexcept { return !actorEditors_.empty(); }
        [[nodiscard]] const std::string& GetActiveNavigationId() const noexcept { return activeNavigationId_; }
        [[nodiscard]] Gui::EditorLayoutType GetActiveLayout() const noexcept { return activeLayout_; }
        [[nodiscard]] const std::vector<std::string>& GetEditorOrder() const noexcept { return actorEditorOrder_; }
        [[nodiscard]] ActorEditorEntry* FindEditor(std::string_view navigationId) noexcept;

    private:
        struct PathHash
        {
            std::size_t operator()(const std::filesystem::path& value) const noexcept
            {
                return std::hash<std::string>{}(value.generic_string());
            }
        };

        static constexpr std::size_t kActorEditorPanelCapacity = 4;
        using PanelBuffer = std::array<Gui::GuiPanel*, kActorEditorPanelCapacity>;
        
        void ApplyActorEditorPanels(ActorEditorEntry& entry);
        [[nodiscard]] std::span<Gui::GuiPanel*> CollectPanels(const ActorEditorPanels& panels, PanelBuffer& buffer) const noexcept;

        void FocusPanel(Gui::GuiPanel* panel) const;
        void RequestSceneFocus() const;

        /** Détache et supprime les panneaux d'un éditeur */
        void DetachAndRemovePanels(const ActorEditorPanels& panels);

        /** Changer de layout et appliquer le focus */
        void SwitchToLayout(Gui::EditorLayoutType layout, std::string_view navId, Gui::GuiPanel* panelToFocus = nullptr);

    private:
        
        Gui::GuiManager* guiManager_{nullptr};
        Gui::GuiLayoutManager* layoutManager_{nullptr};
        SubsystemManager* subsystems_{nullptr};

        FocusRequestCallback focusRequestCallback_{};
        FocusSceneCallback focusSceneCallback_{};

        std::unordered_map<std::string, ActorEditorEntry> actorEditors_{};
        std::unordered_map<std::filesystem::path, std::string, PathHash> actorEditorsByPath_{};
        std::vector<std::string> actorEditorOrder_{};

        std::string activeNavigationId_{"scene"};
        Gui::EditorLayoutType activeLayout_{Gui::EditorLayoutType::Scene};
    };
}

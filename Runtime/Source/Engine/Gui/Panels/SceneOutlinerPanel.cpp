#include "Engine/Gui/Panels/SceneOutlinerPanel.h"

#include "Game/Actor.h"
#include "Game/Scene.h"
#include "Game/SceneManager.h"
#include "Engine/Gui/Internal/GuiManager.h"
#include "Engine/Gui/Internal/GuiPanel.h"
#include "Engine/Gui/Controllers/GuiPanelController.h"
#include "Engine/Gui/Utils/GuiHelpers.h"
#include "Engine/Gui/Widgets/GuiWidgetLibrary.h"
#include "Core/Containers/String.h"

#include "imgui.h"

#include <array>
#include <functional>

#include "Engine/Gui/Internal/GuiDocking.h"

namespace BixEngine::Gui
{
    using namespace Theme;
    using namespace Utils;

    namespace
    {
        class SceneOutlinerPanelController final : public GuiPanelController
        {
        public:
            SceneOutlinerPanelController(std::function<Game::SceneManager*()> sceneProvider,
                                        std::function<Game::Actor*()> selectionGetter,
                                        std::function<void(Game::Actor*)> selectionSetter)
                : sceneManagerProvider_(std::move(sceneProvider))
                , selectedActorGetter_(std::move(selectionGetter))
                , selectedActorSetter_(std::move(selectionSetter))
            {
                searchBuffer_.fill('\0');
            }

        protected:
            void OnAttach(GuiPanel& panel) override
            {
                panel.SetResizable(true);
                panel.SetMovable(true);
                panel.SetCollapsable(true);
                panel.SetClosable(true);
                panel.SetBackgroundColor(OutlinerBackground);
                panel.AddWindowFlags(ImGuiWindowFlags_NoCollapse);
            }

            void OnDraw(GuiPanel&) override
            {
                ScopedID panelScope("SceneOutlinerPanel");

                const Game::SceneManager* sceneManager = sceneManagerProvider_ ? sceneManagerProvider_() : nullptr;
                const Game::Scene* activeScene = sceneManager ? sceneManager->GetScene() : nullptr;
                if (!activeScene)
                {
                    DrawEmptyStateMessage("No active scene.");
                    return;
                }

                Widgets::PanelToolbar toolbar;
                toolbar.AddLeft([this]()
                {
                    SearchInput("SceneOutlinerSearch", searchBuffer_.data(), searchBuffer_.size(), "Search actors...");
                });
                toolbar.Commit();

                const String searchQuery(searchBuffer_.data());
                const bool hasSearch = !searchQuery.IsEmpty();

                const auto& actors = activeScene->GetActors();
                Game::Actor* selectedActor = selectedActorGetter_ ? selectedActorGetter_() : nullptr;

                const auto matchesFilter = [&searchQuery, hasSearch](const Game::Actor& actor)
                {
                    if (!hasSearch)
                        return true;

                    const BixEngine::String& actorName = actor.GetName();
                    if (!actorName.IsEmpty() && actorName.Contains(searchQuery.View(), false))
                        return true;

                    BixEngine::String typeName(actor.GetTypeName());
                    return typeName.Contains(searchQuery.View(), false);
                };

                std::size_t totalActors = 0;
                std::size_t filteredActors = 0;
                for (const auto& actor : actors)
                {
                    if (!actor)
                        continue;

                    ++totalActors;

                    if (matchesFilter(*actor))
                        ++filteredActors;
                }

                const BixEngine::String& sceneName = activeScene->GetName();
                const auto sceneNameView = sceneName.View();
                const ImGuiTreeNodeFlags sceneFlags =
                    ImGuiTreeNodeFlags_DefaultOpen |
                    ImGuiTreeNodeFlags_OpenOnArrow |
                    ImGuiTreeNodeFlags_OpenOnDoubleClick |
                    ImGuiTreeNodeFlags_SpanFullWidth;

                if (ImGui::TreeNodeEx(static_cast<const void*>(activeScene), sceneFlags, "%.*s",
                    static_cast<int>(sceneNameView.size()), sceneNameView.data()))
                {
                    if (ImGui::IsItemClicked() && selectedActorSetter_)
                        selectedActorSetter_(nullptr);

                    if (totalActors == 0)
                    {
                        DrawEmptyStateMessage("No actors in this scene.");
                    }
                    else
                    {
                        for (const auto& actor : actors)
                        {
                            if (!actor || !matchesFilter(*actor))
                                continue;

                            const BixEngine::String& actorName = actor->GetName();
                            const auto actorNameView = actorName.View();
                            const BixEngine::String actorType = actor->GetTypeName();
                            const auto actorTypeView = actorType.View();

                            const bool hasName = !actorNameView.empty();
                            ImGuiTreeNodeFlags actorFlags =
                                ImGuiTreeNodeFlags_Leaf |
                                ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                ImGuiTreeNodeFlags_SpanFullWidth;

                            if (selectedActor == actor.get())
                                actorFlags |= ImGuiTreeNodeFlags_Selected;

                            if (hasName)
                            {
                                ImGui::TreeNodeEx(actor.get(), actorFlags, "%.*s (%.*s)",
                                    static_cast<int>(actorNameView.size()), actorNameView.data(),
                                    static_cast<int>(actorTypeView.size()), actorTypeView.data());
                            }
                            else
                            {
                                ImGui::TreeNodeEx(actor.get(), actorFlags, "<Unnamed> (%.*s)",
                                    static_cast<int>(actorTypeView.size()), actorTypeView.data());
                            }

                            if (ImGui::IsItemClicked() && selectedActorSetter_)
                                selectedActorSetter_(actor.get());
                        }

                        if (hasSearch && filteredActors == 0)
                            DrawEmptyStateMessage("No actors match the current filter.");
                    }

                    ImGui::TreePop();
                }

                ImGui::Spacing();
                if (hasSearch)
                    ImGui::Text("%zu / %zu actor%s", filteredActors, totalActors, totalActors == 1 ? "" : "s");
                else
                    ImGui::Text("%zu actor%s", totalActors, totalActors == 1 ? "" : "s");
            }

        private:
            std::function<Game::SceneManager*()> sceneManagerProvider_{};
            std::function<Game::Actor*()> selectedActorGetter_{};
            std::function<void(Game::Actor*)> selectedActorSetter_{};
            std::array<char, 128> searchBuffer_{};
        };
    }

    GuiPanel& CreateSceneOutlinerPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context)
    {
        auto registration = guiManager.RegisterUtilityPanel<SceneOutlinerPanelController>(
            "scene_outliner",
            "Scene Outliner",
            context.sceneManagerProvider,
            context.selectedActorGetter,
            context.selectedActorSetter);

        guiManager.SetPanelDockingArea(registration.panel, DockSpaceRegion::Left);
        return registration.panel;
    }
}

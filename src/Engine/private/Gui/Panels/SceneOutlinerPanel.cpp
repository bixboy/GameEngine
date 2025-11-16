#include "Gui/Panels/SceneOutlinerPanel.h"

#include <utility>

#include "Actor.h"
#include "Scene.h"
#include "SceneManager.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Gui/Widgets/Widgets.h"

#include "imgui.h"

namespace BixEngine::Gui
{
    using namespace Utils;

    SceneOutlinerPanel::SceneOutlinerPanel(std::function<Game::SceneManager*()> sceneProvider, std::function<Game::Actor*()> selectionGetter, std::function<void(Game::Actor*)> selectionSetter)
        : GuiPanelBase("scene_outliner"),
        sceneManagerProvider_(std::move(sceneProvider)),
        selectedActorGetter_(std::move(selectionGetter)),
        selectedActorSetter_(std::move(selectionSetter))
    {
        searchBuffer_.fill('\0');
    }

    SceneOutlinerPanel::SceneOutlinerPanel(const DefaultEngineGuiContext& context)
        : SceneOutlinerPanel(context.sceneManagerProvider, context.selectedActorGetter, context.selectedActorSetter)
    {
    }

    void SceneOutlinerPanel::Draw()
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

            const String& actorName = actor.GetName();
            if (!actorName.IsEmpty() && actorName.Contains(searchQuery.View(), false))
                return true;

            String typeName(actor.GetTypeName());
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

        const String& sceneName = activeScene->GetName();
        const auto sceneNameView = sceneName.View();
        constexpr ImGuiTreeNodeFlags sceneFlags =
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_SpanFullWidth;

        if (ImGui::TreeNodeEx(activeScene, sceneFlags, "%.*s",
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

                    const String& actorName = actor->GetName();
                    const auto actorNameView = actorName.View();
                    const String actorType = actor->GetTypeName();
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
}

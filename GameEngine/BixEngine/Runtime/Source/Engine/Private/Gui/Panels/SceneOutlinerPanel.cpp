#include "Bix/Engine/Gui/Panels/SceneOutlinerPanel.h"

#include "Bix/Game/Actor.h"
#include "Bix/Game/Scene.h"
#include "Bix/Game/SceneManager.h"
#include "Bix/Engine/Gui/GuiManager.h"
#include "Bix/Engine/Gui/GuiPanel.h"

#include "imgui.h"

#include <cstddef>

namespace BixEngine::Gui
{
    namespace
    {
        constexpr ImVec4 kOutlinerBackground{0.11f, 0.11f, 0.11f, 0.95f};
    }

    GuiPanel& CreateSceneOutlinerPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context)
    {
        GuiPanel& outlinerPanel = guiManager.CreatePanel("scene_outliner", "Scene Outliner");
        guiManager.SetPanelDockingArea(outlinerPanel, DockSpaceRegion::Left);
        outlinerPanel.SetResizable(true);
        outlinerPanel.SetMovable(true);
        outlinerPanel.SetCollapsable(true);
        outlinerPanel.SetClosable(true);
        outlinerPanel.SetBackgroundColor(kOutlinerBackground);
        outlinerPanel.AddWindowFlags(ImGuiWindowFlags_NoCollapse);
        outlinerPanel.SetDrawFunction([provider = context.sceneManagerProvider,
                                       getSelectedActor = context.selectedActorGetter,
                                       setSelectedActor = context.selectedActorSetter]()
        {
            ImGui::PushID("SceneOutlinerPanel");

            const Game::SceneManager* sceneManager = provider ? provider() : nullptr;
            const Game::Scene* activeScene = sceneManager ? sceneManager->GetScene() : nullptr;
            if (!activeScene)
            {
                ImGui::TextDisabled("No active scene.");
                ImGui::PopID();
                return;
            }

            static char searchBuffer[128] = "";
            ImGui::InputTextWithHint("##SceneOutlinerSearch", "Search actors...", searchBuffer, IM_ARRAYSIZE(searchBuffer));
            const String searchQuery(searchBuffer);
            const bool hasSearch = !searchQuery.IsEmpty();

            const auto& actors = activeScene->GetActors();
            Game::Actor* selectedActor = getSelectedActor ? getSelectedActor() : nullptr;

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

            ImGui::Separator();

            const BixEngine::String& sceneName = activeScene->Name();
            const auto sceneNameView = sceneName.View();
            const ImGuiTreeNodeFlags sceneFlags =
                ImGuiTreeNodeFlags_DefaultOpen |
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_OpenOnDoubleClick |
                ImGuiTreeNodeFlags_SpanFullWidth;

            if (ImGui::TreeNodeEx(static_cast<const void*>(activeScene), sceneFlags, "%.*s",
                static_cast<int>(sceneNameView.size()), sceneNameView.data()))
            {
                if (ImGui::IsItemClicked() && setSelectedActor)
                    setSelectedActor(nullptr);

                if (totalActors == 0)
                {
                    ImGui::TextDisabled("No actors in this scene.");
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

                        if (ImGui::IsItemClicked() && setSelectedActor)
                            setSelectedActor(actor.get());
                    }

                    if (hasSearch && filteredActors == 0)
                        ImGui::TextDisabled("No actors match the current filter.");
                }

                ImGui::TreePop();
            }

            ImGui::Separator();

            if (hasSearch)
                ImGui::Text("%zu / %zu actor%s", filteredActors, totalActors, totalActors == 1 ? "" : "s");
            else
                ImGui::Text("%zu actor%s", totalActors, totalActors == 1 ? "" : "s");

            ImGui::PopID();
        });

        return outlinerPanel;
    }
}

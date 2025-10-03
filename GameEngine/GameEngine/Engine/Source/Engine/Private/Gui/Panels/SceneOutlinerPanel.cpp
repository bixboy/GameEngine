#include "Gui/Panels/SceneOutlinerPanel.h"

#include "Game/Actor.h"
#include "Game/Scene.h"
#include "Game/SceneManager.h"
#include "Gui/GuiManager.h"
#include "Gui/GuiPanel.h"

#include "imgui.h"

#include <cstddef>
#include <string_view>

namespace Engine::Gui
{
    namespace
    {
        constexpr ImVec4 kOutlinerBackground{0.11f, 0.11f, 0.11f, 0.95f};
    }

    GuiPanel& CreateSceneOutlinerPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context)
    {
        GuiPanel& outlinerPanel = guiManager.CreatePanel("scene_outliner", "Scene Outliner");
        outlinerPanel.SetPosition(0.f, 260.0f);
        outlinerPanel.SetSize(320.0f, 400.0f);
        outlinerPanel.SetResizable(true);
        outlinerPanel.SetMovable(true);
        outlinerPanel.SetCollapsable(true);
        outlinerPanel.SetClosable(true);
        outlinerPanel.SetBackgroundColor(kOutlinerBackground);
        outlinerPanel.AddWindowFlags(ImGuiWindowFlags_NoCollapse);
        outlinerPanel.SetDrawFunction([provider = context.sceneManagerProvider]()
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
            const std::string_view searchQuery(searchBuffer);
            const bool hasSearch = !searchQuery.empty();

            const auto& actors = activeScene->GetActors();

            const auto matchesFilter = [&searchQuery, hasSearch](const Game::Actor& actor)
            {
                if (!hasSearch)
                    return true;

                const Engine::String& actorName = actor.GetName();
                if (!actorName.IsEmpty() && actorName.Contains(searchQuery, false))
                    return true;

                Engine::String typeName(actor.GetTypeName());
                return typeName.Contains(searchQuery, false);
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

            const std::string_view sceneName = activeScene->Name();
            const ImGuiTreeNodeFlags sceneFlags =
                ImGuiTreeNodeFlags_DefaultOpen |
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_OpenOnDoubleClick |
                ImGuiTreeNodeFlags_SpanFullWidth;

            if (ImGui::TreeNodeEx(static_cast<const void*>(activeScene), sceneFlags, "%.*s",
                static_cast<int>(sceneName.size()), sceneName.data()))
            {
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

                        const Engine::String& actorName = actor->GetName();
                        const std::string_view actorNameView = actorName.View();
                        const std::string_view actorType = actor->GetTypeName();

                        const bool hasName = !actorNameView.empty();
                        const ImGuiTreeNodeFlags actorFlags =
                            ImGuiTreeNodeFlags_Leaf |
                            ImGuiTreeNodeFlags_NoTreePushOnOpen |
                            ImGuiTreeNodeFlags_SpanFullWidth;

                        if (hasName)
                        {
                            ImGui::TreeNodeEx(actor.get(), actorFlags, "%.*s (%.*s)",
                                static_cast<int>(actorNameView.size()), actorNameView.data(),
                                static_cast<int>(actorType.size()), actorType.data());
                        }
                        else
                        {
                            ImGui::TreeNodeEx(actor.get(), actorFlags, "<Unnamed> (%.*s)",
                                static_cast<int>(actorType.size()), actorType.data());
                        }
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

#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserUI_Tree.h"

#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserState.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserFileUtils.h"
#include "Engine/Gui/Utils/GuiHelpers.h"

#include "imgui.h"

#include <vector>

namespace BixEngine::Gui
{
    namespace
    {
        constexpr float kContentTreeWidth = 240.0f;
        constexpr ImVec4 kContentTreeBackground{0.13f, 0.13f, 0.13f, 0.95f};
    }

    // ─────────────────────────────────────────────
    // 🌳  Arborescence des dossiers
    // ─────────────────────────────────────────────

    void RenderDirectoryTree(ContentBrowserState& state, String& selectedEntry)
    {
        namespace fs = std::filesystem;
        namespace Utils = Gui::Utils;

        ScopedStyle treeSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f));
        if (!ImGui::BeginChild("ContentBrowserTree", ImVec2(kContentTreeWidth, 0.0f), true))
            return;

        ScopedColor treeColor(ImGuiCol_ChildBg, kContentTreeBackground);
        if (ImGui::BeginChild("ContentBrowserTreeInner", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_AlwaysVerticalScrollbar))
        {
            const auto renderDirectoryTree = [&](auto&& self, const fs::path& directory, int depth) -> void
            {
                const String directoryName = directory == state.root ? String("Content") : String(directory.filename().generic_string());
                const String directoryId = directory.generic_string();
                std::error_code equivalentError;
                const bool isSelected = fs::equivalent(directory, state.current, equivalentError);
                const ImGuiTreeNodeFlags nodeFlags =
                    ImGuiTreeNodeFlags_OpenOnArrow |
                    ImGuiTreeNodeFlags_OpenOnDoubleClick |
                    ImGuiTreeNodeFlags_SpanFullWidth |
                    (isSelected ? ImGuiTreeNodeFlags_Selected : 0);

                const bool open = ImGui::TreeNodeEx(directoryId.c_str(), nodeFlags, "%s", directoryName.c_str());
                if (ImGui::IsItemClicked())
                {
                    state.current = directory;
                    selectedEntry.Clear();
                }

                if (open)
                {
                    std::vector<fs::path> children{};
                    std::error_code childError;
                    for (const auto& entry : fs::directory_iterator(directory, childError))
                    {
                        if (!entry.is_directory())
                            continue;

                        children.push_back(entry.path());
                    }

                    if (childError)
                    {
                        Utils::DrawEmptyStateMessage("Unable to open directory.");
                    }
                    else
                    {
                        std::sort(children.begin(), children.end(), [&](const fs::path& lhs, const fs::path& rhs)
                        {
                            const String lhsName = lhs.filename().generic_string();
                            const String rhsName = rhs.filename().generic_string();
                            return CaseInsensitiveLess(lhsName, rhsName);
                        });

                        for (const auto& child : children)
                        {
                            const int nextDepth = depth + 1;
                            if (nextDepth > 64)
                            {
                                Utils::DrawEmptyStateMessage("...");
                                break;
                            }

                            self(self, child, nextDepth);
                        }
                    }

                    ImGui::TreePop();
                }
            };

            if (fs::exists(state.root))
                renderDirectoryTree(renderDirectoryTree, state.root, 0);
        }

        ImGui::EndChild();
        ImGui::EndChild();
    }
}


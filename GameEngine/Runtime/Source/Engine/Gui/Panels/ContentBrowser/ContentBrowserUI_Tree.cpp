#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserUI_Tree.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserState.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserFileUtils.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentEntry.h"
#include "Engine/Gui/Utils/GuiHelpers.h"
#include "imgui.h"
#include <vector>
#include <unordered_map>


namespace BixEngine::Gui
{
    using namespace Theme;
    using namespace Utils;

    namespace
    {
        struct DirectoryChildrenCache
        {
            std::unordered_map<std::string, std::vector<std::filesystem::path>> children;
            void Clear() { children.clear(); }
        };

        DirectoryChildrenCache gCache;

        bool gRequestExpandAll = false;
        bool gRequestCollapseAll = false;
    }

    void RenderDirectoryTree(ContentBrowserState& state, String& selectedEntry)
    {
        namespace fs = std::filesystem;

        ScopedStyle treeSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f));
        if (!ImGui::BeginChild("ContentBrowserTree", ImVec2(ContentTreeWidth, 0.0f), true))
        {
            ImGui::EndChild();
            return;
        }

        {
            ScopedColor treeColor(ImGuiCol_ChildBg, ContentTreeBackground);

            // ─────────────────────────────────────────────
            // 🧭 Toolbar minimaliste
            // ─────────────────────────────────────────────
            {
                ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(2.5f, 2.5f));
                ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));
                ScopedStyle rounding(ImGuiStyleVar_FrameRounding, 3.0f);

                const float buttonSize = 22.0f;
                const ImVec4 baseColor = ImVec4(0.25f, 0.25f, 0.25f, 0.8f);
                const ImVec4 hoverColor = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
                const ImVec4 activeColor = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);

                ScopedColor buttonColor(ImGuiCol_Button, baseColor);
                ScopedColor buttonHover(ImGuiCol_ButtonHovered, hoverColor);
                ScopedColor buttonActive(ImGuiCol_ButtonActive, activeColor);

                ScopedStyle innerSpacing(ImGuiStyleVar_ItemInnerSpacing, ImVec2(4.f, 2.f));

                // Collapse
                if (ImGui::Button("➖", ImVec2(buttonSize, buttonSize)))
                {
                    gRequestCollapseAll = true;
                    gRequestExpandAll = false;
                }
                
                ShowTooltip("Collapse all folders");

                ImGui::SameLine();

                // Expand
                if (ImGui::Button("➕", ImVec2(buttonSize, buttonSize)))
                {
                    gRequestExpandAll = true;
                    gRequestCollapseAll = false;
                }
                
                ShowTooltip("Expand all folders");

                ImGui::SameLine();

                // Refresh
                if (ImGui::Button("🔄", ImVec2(buttonSize, buttonSize)))
                {
                    gCache.Clear();
                }
                
                ShowTooltip("Refresh directory");

                ImGui::Dummy(ImVec2(0.0f, 4.0f));
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0.0f, 2.0f));
            }


            // ─────────────────────────────────────────────
            // Arborescence interne scrollable
            // ─────────────────────────────────────────────
            if (ImGui::BeginChild("ContentBrowserTreeInner", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_AlwaysVerticalScrollbar))
            {
                const auto renderDirectoryTree = [&](auto&& self, const fs::path& directory, int depth) -> void
                {
                    const String directoryName = (directory == state.root) ? "Content" : directory.filename().generic_string();
                    const String directoryId = directory.generic_string();

                    std::error_code err;
                    const bool isSelected = fs::equivalent(directory, state.current, err);
                    if (err)
                        return;

                    if (gRequestExpandAll || gRequestCollapseAll)
                        ImGui::SetNextItemOpen(gRequestExpandAll, ImGuiCond_Always);

                    const ImGuiTreeNodeFlags nodeFlags =
                        ImGuiTreeNodeFlags_OpenOnArrow |
                        ImGuiTreeNodeFlags_OpenOnDoubleClick |
                        ImGuiTreeNodeFlags_SpanFullWidth |
                        (isSelected ? ImGuiTreeNodeFlags_Selected : 0);

                    const char* icon = GetIcon(ContentType::Directory);
                    const bool open = ImGui::TreeNodeEx(directoryId.c_str(), nodeFlags, "%s  %s", icon, directoryName.c_str());

                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                    {
                        state.current = directory;
                        selectedEntry.Clear();
                    }

                    if (open)
                    {
                        auto& children = gCache.children[directoryId.c_str()];
                        if (children.empty())
                        {
                            std::error_code childErr;
                            for (const auto& entry : fs::directory_iterator(directory, childErr))
                            {
                                if (entry.is_directory())
                                    children.push_back(entry.path());   
                            }

                            std::sort(children.begin(), children.end(),
                                [](const fs::path& a, const fs::path& b)
                                {
                                    return CaseInsensitiveLess(a.filename().generic_string(), b.filename().generic_string());
                                });
                        }

                        for (const auto& child : children)
                        {
                            if (depth > 64)
                            {
                                DrawEmptyStateMessage("...");
                                break;
                            }
                            self(self, child, depth + 1);
                        }

                        ImGui::TreePop();
                    }
                };

                if (fs::exists(state.root))
                    renderDirectoryTree(renderDirectoryTree, state.root, 0);
                else
                    DrawEmptyStateMessage("Content directory not found.");

                ImGui::EndChild();
            }
        }

        ImGui::EndChild();

        gRequestExpandAll   = false;
        gRequestCollapseAll = false;
    }
}

#include "Engine/Gui/Utils/GuiHelpers.h"

#include <cfloat>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    using namespace BixEngine::Gui::Utils;

    int g_smallFontPushCount = 0;
    std::unordered_map<std::string, bool> g_persistentSections;
    std::vector<std::string> g_persistentSectionStack;

    /// Retrieves the smallest available font registered in ImGui.
	ImFont* GetSmallestFont()
    {
        ImGuiIO& io = ImGui::GetIO();
        ImFont* current  = ImGui::GetFont();
        ImFont* smallest = current;
        float    best    = FLT_MAX;

        for (ImFont* font : io.Fonts->Fonts)
        {
            if (!font) continue;

            ImGui::PushFont(font);
            const float sz = ImGui::GetFontSize();
            ImGui::PopFont();

            if (sz < best)
            {
                best = sz;
                smallest = font;
            }
        }

        return smallest ? smallest : current;
    }

    /// Internal recursive drawing helper.
    void DrawTreeNodeRecursive(const TreeNodeData& node, std::string& selectedNode, const TreeNodeCallback& onContextMenu)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (node.isLeaf || node.children.empty())
        {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }
        if (node.name == selectedNode)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        const bool open = ImGui::TreeNodeEx(node.name.c_str(), flags);
        if (ImGui::IsItemClicked())
        {
            selectedNode = node.name;
        }

        if (ImGui::BeginPopupContextItem())
        {
            if (onContextMenu)
            {
                onContextMenu(node);
            }
            ImGui::EndPopup();
        }

        if (open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
        {
            for (size_t childIndex = 0; childIndex < node.children.size(); ++childIndex)
            {
                ImGui::PushID(static_cast<int>(childIndex));
                DrawTreeNodeRecursive(node.children[childIndex], selectedNode, onContextMenu);
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }
}

namespace BixEngine::Gui::Utils
{
    void DrawSectionHeader(const char* title)
    {
        ImGui::Spacing();
        ImGui::Separator();
        PushSmallFont();
        ImGui::TextDisabled("%s", title ? title : "");
        PopSmallFont();
        ImGui::Separator();
        ImGui::Spacing();
    }

    void DrawErrorMessage(const std::string& message)
    {
        if (message.empty())
        {
            return;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        ImGui::TextWrapped("%s", message.c_str());
        ImGui::PopStyleColor();
    }

    bool DrawConfirmButtons(const char* okLabel, const char* cancelLabel)
    {
        return DrawConfirmButtons(okLabel, cancelLabel, nullptr, nullptr);
    }

    bool DrawConfirmButtons(const char* okLabel, const char* cancelLabel, const std::function<void()>& onConfirm, const std::function<void()>& onCancel)
    {
        bool confirmed = false;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, ImGui::GetStyle().ItemSpacing.y));

        const char* confirmLabel = okLabel ? okLabel : "Confirm";
        const char* cancelLabelSafe = cancelLabel ? cancelLabel : "Cancel";

        if (ImGui::Button(confirmLabel))
        {
            confirmed = true;
            if (onConfirm)
            {
                onConfirm();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button(cancelLabelSafe))
        {
            if (onCancel)
            {
                onCancel();
            }
        }

        ImGui::PopStyleVar();

        return confirmed;
    }

    bool InputTextWithLabel(const char* label, char* buffer, size_t bufferSize, ImGuiInputTextFlags flags, bool autoFocus)
    {
        if (!buffer || bufferSize == 0)
        {
            return false;
        }

        const char* caption = label ? label : "";

        ImGui::PushID(caption);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(caption);
        ImGui::SameLine();
        const float availableWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(availableWidth > 0.0f ? availableWidth : 0.0f);

        if (autoFocus && ImGui::IsWindowAppearing())
        {
            ImGui::SetKeyboardFocusHere();
        }

        const bool edited = ImGui::InputText("##Value", buffer, bufferSize, flags);
        ImGui::PopID();
        return edited;
    }

    bool InputTextWithLabelValidated(const char* label, char* buffer, size_t bufferSize, const std::function<bool(const char*)>& validator, ImGuiInputTextFlags flags, bool autoFocus, bool* outIsValid)
    {
        if (!buffer || bufferSize == 0)
        {
            if (outIsValid)
            {
                *outIsValid = false;
            }
            return false;
        }

        const bool edited = InputTextWithLabel(label, buffer, bufferSize, flags, autoFocus);

        bool isValid = validator ? validator(buffer) : true;

        if (!isValid)
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const ImVec2 min = ImGui::GetItemRectMin();
            const ImVec2 max = ImGui::GetItemRectMax();
            const float rounding = ImGui::GetStyle().FrameRounding;
            drawList->AddRect(min, max, ImGui::GetColorU32(ImVec4(0.9f, 0.2f, 0.2f, 1.0f)), rounding, 0, 2.0f);

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            {
                ImGui::SetTooltip("Invalid value");
            }
        }

        if (outIsValid)
        {
            *outIsValid = isValid;
        }

        return edited;
    }

    void DrawDescriptionText(const char* text)
    {
        if (!text || text[0] == '\0')
        {
            return;
        }

        ImGui::TextWrapped("%s", text);
    }

    void DrawLabelValue(const char* label, const std::string& value, const char* emptyFallback)
    {
        const char* caption = label ? label : "";
        const char* displayed = (!value.empty() ? value.c_str() : (emptyFallback ? emptyFallback : ""));
        ImGui::Text("%s: %s", caption, displayed);
    }

    void DrawEmptyStateMessage(const char* message)
    {
        ImGui::TextDisabled("%s", message ? message : "");
    }

    void DrawScrollableList(const std::vector<std::string>& items, float height, const std::string& selected, std::string& outSelection)
    {
        DrawScrollableList(items, height, selected, outSelection, nullptr);
    }

    void DrawScrollableList(const std::vector<std::string>& items, float height, const std::string& selected, std::string& outSelection, const ListItemCallback& onItemHover)
    {
        ImVec2 size = ImVec2(0.0f, height);
        if (height <= 0.0f)
        {
            size.y = ImGui::GetTextLineHeightWithSpacing() * 6.0f;
        }

        outSelection = selected;

        if (ImGui::BeginChild("ScrollableList", size, true, ImGuiWindowFlags_HorizontalScrollbar))
        {
            for (size_t index = 0; index < items.size(); ++index)
            {
                const std::string& item = items[index];
                ImGui::PushID(static_cast<int>(index));
                const bool isSelected = item == selected;
                if (ImGui::Selectable(item.c_str(), isSelected))
                {
                    outSelection = item;
                }
                if (onItemHover && ImGui::IsItemHovered())
                {
                    onItemHover(item);
                }
                ImGui::PopID();
            }
        }
        ImGui::EndChild();
    }

    void DrawTreeRecursive(const std::vector<TreeNodeData>& roots, std::string& selectedNode)
    {
        DrawTreeRecursive(roots, selectedNode, nullptr);
    }

    void DrawTreeRecursive(const std::vector<TreeNodeData>& roots, std::string& selectedNode, const TreeNodeCallback& onContextMenu)
    {
        for (size_t nodeIndex = 0; nodeIndex < roots.size(); ++nodeIndex)
        {
            ImGui::PushID(static_cast<int>(nodeIndex));
            DrawTreeNodeRecursive(roots[nodeIndex], selectedNode, onContextMenu);
            ImGui::PopID();
        }
    }

    void DrawScriptHierarchyTree(const std::vector<TreeNodeData>& roots, std::string& selectedNode, const char* emptyMessage)
    {
        DrawScriptHierarchyTree(roots, selectedNode, nullptr, emptyMessage);
    }

    void DrawScriptHierarchyTree(const std::vector<TreeNodeData>& roots, std::string& selectedNode, const TreeNodeCallback& onContextMenu, const char* emptyMessage)
    {
        if (roots.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::TextUnformatted(emptyMessage ? emptyMessage : "");
            ImGui::PopStyleColor();
            return;
        }

        ImGui::PushID("ScriptHierarchyTree");
        DrawTreeRecursive(roots, selectedNode, onContextMenu);
        ImGui::PopID();
    }

    bool BeginCollapsibleSection(const char* label, bool defaultOpen, ImGuiTreeNodeFlags additionalFlags)
    {
        ImGuiTreeNodeFlags flags = additionalFlags;
        if (defaultOpen)
            flags |= ImGuiTreeNodeFlags_DefaultOpen;

        const char* headerLabel = label ? label : "";
        const bool open = ImGui::CollapsingHeader(headerLabel, flags);

        if (!open)
        {
            ImGui::Dummy(ImVec2(0, 0));
            return false;
        }

        return true;
    }

    bool BeginPersistentSection(const char* label, const std::string& contextId, bool defaultOpen, ImGuiTreeNodeFlags additionalFlags)
    {
        if (!label)
        {
            return false;
        }

        std::string key = contextId;
        if (!key.empty())
        {
            key.append("::");
        }
        key.append(label);

        auto [it, inserted] = g_persistentSections.emplace(key, defaultOpen);
        bool& isOpen = it->second;

        if (inserted)
        {
            ImGui::SetNextItemOpen(defaultOpen, ImGuiCond_Once);
        }
        else
        {
            ImGui::SetNextItemOpen(isOpen, ImGuiCond_Always);
        }

        const bool visible = ImGui::CollapsingHeader(label, additionalFlags);
        isOpen = visible;

        if (!visible)
        {
            ImGui::Dummy(ImVec2(0, 0));
            return false;
        }

        g_persistentSectionStack.push_back(key);
        ImGui::PushID(key.c_str());
        return true;
    }

    void EndPersistentSection()
    {
        if (g_persistentSectionStack.empty())
            return;

        ImGui::PopID();
        g_persistentSectionStack.pop_back();
        ImGui::Dummy(ImVec2(0, 0));
    }

    bool SearchInput(const char* id, char* buffer, size_t bufferSize, const char* hint, float width, ImGuiInputTextFlags flags)
    {
        if (!buffer || bufferSize == 0)
        {
            return false;
        }

        const std::string label = id ? std::string("##") + id : std::string("##SearchInput");

        if (width >= 0.0f)
        {
            ImGui::SetNextItemWidth(width);
        }
        else
        {
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        }

        return ImGui::InputTextWithHint(label.c_str(), hint ? hint : "", buffer, bufferSize, flags);
    }

    void PushSmallFont()
    {
        ImGui::PushFont(GetSmallestFont());
        ++g_smallFontPushCount;
    }

    void PopSmallFont()
    {
        if (g_smallFontPushCount > 0)
        {
            ImGui::PopFont();
            --g_smallFontPushCount;
        }
    }

    void DrawSeparatorText(const char* text)
    {
#if IMGUI_VERSION_NUM >= 18967
        ImGui::SeparatorText(text ? text : "");
#else
        const char* label = text ? text : "";
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextDisabled("%s", label);
        ImGui::Separator();
        ImGui::Spacing();
#endif
    }

    bool IconButton(const char* icon, const char* tooltip)
    {
        const char* label = icon ? icon : "";
        ImVec2 size = ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
        const bool pressed = ImGui::Button(label, size);
        ImGui::PopStyleVar();

        if (tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        {
            ImGui::SetTooltip("%s", tooltip);
        }
        return pressed;
    }

    void DrawHelpMarker(const char* text)
    {
        ImGui::TextDisabled("%s", "?");
        if (text && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        {
            ImGui::SetTooltip("%s", text);
        }
    }
}


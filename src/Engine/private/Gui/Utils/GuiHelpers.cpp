#include "Gui/Utils/GuiHelpers.h"

#include <algorithm>
#include <cfloat>
#include <cstdint>
#include <type_traits>

namespace
{
    using namespace BixEngine::Gui::GuiUtils;
    
    ImFont* GetSmallestFontCached()
    {
        auto& state = GuiStateManager::Get();
        if (state.CachedSmallestFont)
            return state.CachedSmallestFont;

        ImGuiIO& io = ImGui::GetIO();
        ImFont* smallest = ImGui::GetFont();
        float best = FLT_MAX;

        for (ImFont* font : io.Fonts->Fonts)
        {
            if (!font)
                continue;
            
            ImGui::PushFont(font);
            float size = ImGui::GetFontSize();
            ImGui::PopFont();
            
            if (size < best)
            {
                best = size;
                smallest = font;
            }
        }

        state.CachedSmallestFont = smallest;
        return smallest;
    }
    
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

        bool open = ImGui::TreeNodeEx(node.name.c_str(), flags);
        if (ImGui::IsItemClicked())
            selectedNode = node.name;

        if (ImGui::BeginPopupContextItem())
        {
            if (onContextMenu)
                onContextMenu(node);

            ImGui::EndPopup();
        }

        if (open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
        {
            for (size_t i = 0; i < node.children.size(); ++i)
            {
                ScopedID id(static_cast<int>(i));
                DrawTreeNodeRecursive(node.children[i], selectedNode, onContextMenu);
            }

            ImGui::TreePop();
        }
    }
}

namespace BixEngine::Gui::GuiUtils
{
    
    void DrawSectionHeader(const char* title)
    {
        ImGui::Spacing();
        ImGui::Separator();
        ScopedFont small(GetSmallestFontCached());
        ImGui::TextDisabled("%s", title ? title : "");
        ImGui::Separator();
        ImGui::Spacing();
    }

    void DrawErrorMessage(const std::string& message)
    {
        if (message.empty())
            return;
        
        ScopedColor color(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.4f, 1.f));
        ImGui::TextWrapped("%s", message.c_str());
    }

    bool DrawConfirmButtons(const char* okLabel, const char* cancelLabel)
    {
        return DrawConfirmButtons(okLabel, cancelLabel, nullptr, nullptr);
    }

    bool DrawConfirmButtons(const char* okLabel, const char* cancelLabel, const std::function<void()>& onConfirm, const std::function<void()>& onCancel)
    {
        bool confirmed = false;
        ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, ImGui::GetStyle().ItemSpacing.y));

        if (ImGui::Button(okLabel ? okLabel : "Confirm"))
        {
            confirmed = true;
            if (onConfirm)
                onConfirm();
        }

        ImGui::SameLine();

        if (ImGui::Button(cancelLabel ? cancelLabel : "Cancel"))
        {
            if (onCancel)
                onCancel();
        }

        return confirmed;
    }
    
    bool InputTextWithLabel(const char* label, char* buffer, size_t bufferSize, ImGuiInputTextFlags flags,
                            bool autoFocus)
    {
        if (!buffer || bufferSize == 0)
            return false;

        ScopedID id(label ? label : "##Input");
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label ? label : "");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

        if (autoFocus && ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();

        return ImGui::InputText("##Value", buffer, bufferSize, flags);
    }

    bool InputTextWithLabelValidated(const char* label, char* buffer, size_t bufferSize,const std::function<bool(const char*)>& validator,
        ImGuiInputTextFlags flags, bool autoFocus, bool* outIsValid)
    {
        if (!buffer || bufferSize == 0)
        {
            if (outIsValid) *outIsValid = false;
            return false;
        }

        bool edited = InputTextWithLabel(label, buffer, bufferSize, flags, autoFocus);
        bool isValid = validator ? validator(buffer) : true;

        if (!isValid)
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();
            drawList->AddRect(min, max, IM_COL32(230, 40, 40, 255), ImGui::GetStyle().FrameRounding, 0, 2.f);

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
                ImGui::SetTooltip("Invalid value");
        }

        if (outIsValid)
            *outIsValid = isValid;

        return edited;
    }

    
    
    
    void DrawDescriptionText(const char* text)
    {
        if (text && *text)
            ImGui::TextWrapped("%s", text);
    }

    void DrawLabelValue(const char* label, const std::string& value, const char* emptyFallback)
    {
        ImGui::Text("%s: %s", label ? label : "", !value.empty() ? value.c_str() : (emptyFallback ? emptyFallback : ""));
    }

    void DrawEmptyStateMessage(const char* message)
    {
        ScopedColor color(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.f));
        ImGui::TextDisabled("%s", message ? message : "");
    }

    
    
    
    void DrawScrollableList(const std::vector<std::string>& items, float height, const std::string& selected, std::string& outSelection)
    {
        DrawScrollableList(items, height, selected, outSelection, nullptr);
    }

    void DrawScrollableList(const std::vector<std::string>& items, float height, const std::string& selected, std::string& outSelection,
        const ListItemCallback& onItemHover)
    {
        outSelection = selected;
        ImVec2 size(0.f, height > 0.f ? height : ImGui::GetTextLineHeightWithSpacing() * 6.f);

        if (ImGui::BeginChild("ScrollableList", size, true))
        {
            for (size_t i = 0; i < items.size(); ++i)
            {
                ScopedID id(static_cast<int>(i));
                bool selectedNow = (items[i] == selected);
                if (ImGui::Selectable(items[i].c_str(), selectedNow))
                    outSelection = items[i];
                
                if (onItemHover && ImGui::IsItemHovered())
                    onItemHover(items[i]);
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
        for (size_t i = 0; i < roots.size(); ++i)
        {
            ScopedID id(static_cast<int>(i));
            DrawTreeNodeRecursive(roots[i], selectedNode, onContextMenu);
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
            DrawEmptyStateMessage(emptyMessage);
            return;
        }

        ScopedID id("ScriptTree");
        DrawTreeRecursive(roots, selectedNode, onContextMenu);
    }

    
    
    

    bool BeginCollapsibleSection(const char* label, bool defaultOpen, ImGuiTreeNodeFlags additionalFlags)
    {
        ImGuiTreeNodeFlags flags = additionalFlags;
        if (defaultOpen)
            flags |= ImGuiTreeNodeFlags_DefaultOpen;

        return ImGui::CollapsingHeader(label ? label : "", flags);
    }

    bool BeginPersistentSection(const char* label, const std::string& contextId, bool defaultOpen, ImGuiTreeNodeFlags additionalFlags)
    {
        auto& state = GuiStateManager::Get();
        std::string key = contextId.empty() ? label : contextId + "::" + label;

        auto [it, inserted] = state.PersistentSections.emplace(key, defaultOpen);
        bool& open = it->second;

        ImGui::SetNextItemOpen(open, ImGuiCond_Always);
        bool visible = ImGui::CollapsingHeader(label, additionalFlags);
        open = visible;

        if (!visible)
            return false;

        state.SectionStack.push_back(key);
        ImGui::PushID(key.c_str());

        return true;
    }

    void EndPersistentSection()
    {
        auto& state = GuiStateManager::Get();
        if (state.SectionStack.empty())
            return;

        ImGui::PopID();
        state.SectionStack.pop_back();
    }

    
    
    

    bool SearchInput(const char* id, char* buffer, size_t bufferSize, const char* hint, float width, ImGuiInputTextFlags flags)
    {
        if (!buffer || bufferSize == 0)
            return false;

        std::string label = id ? std::string("##") + id : "##SearchInput";

        if (width >= 0.0f)
        {
            ImGui::SetNextItemWidth(width);
        }
        else
        {
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        }

        return ImGui::InputTextWithHint(label.c_str(), hint ? hint : "Search...", buffer, bufferSize, flags);
    }

    
    
    
    bool IconButton(const char* icon, const char* tooltip)
    {
        const char* label = icon ? icon : "";
        ImVec2 size(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());

        ScopedStyle pad(ImGuiStyleVar_FramePadding, ImVec2(4.f, 4.f));

        bool pressed = ImGui::Button(label, size);

        if (tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("%s", tooltip);

        return pressed;
    }

    void DrawHelpMarker(const char* text)
    {
        ImGui::TextDisabled("%s", "?");

        if (text && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("%s", text);
    }

    
    
    

    void PushSmallFont()
    {
        if (ImFont* font = GetSmallestFontCached())
        {
            ImGui::PushFont(font);
            GuiStateManager::Get().SmallFontPushCount++;
        }
    }

    void PopSmallFont()
    {
        auto& state = GuiStateManager::Get();
        if (state.SmallFontPushCount > 0)
        {
            ImGui::PopFont();
            state.SmallFontPushCount--;
        }
    }

    void DrawSeparatorText(const char* text)
    {
        ImGui::SeparatorText(text ? text : "");
    }

    ImVec4 AdjustColor(const ImVec4& color, float delta) noexcept
    {
        const auto clamp = [](float value)
        {
            return std::clamp(value, 0.0f, 1.0f);
        };

        return ImVec4(
            clamp(color.x + delta),
            clamp(color.y + delta),
            clamp(color.z + delta),
            clamp(color.w)
        );
    }

    void ShowTooltip(const char* text, ImGuiHoveredFlags flags)
    {
        if (!text || text[0] == '\0')
            return;

        if (ImGui::IsItemHovered(flags))
            ImGui::SetTooltip("%s", text);
    }

    bool IsItemDoubleClicked(ImGuiMouseButton button, ImGuiHoveredFlags flags) noexcept
    {
        return ImGui::IsItemHovered(flags) && ImGui::IsMouseDoubleClicked(button);
    }

    ImTextureRef ToTextureRef(void* nativeHandle)
    {
        if (!nativeHandle)
            return ImTextureRef();

        if constexpr (std::is_pointer_v<ImTextureID>)
        {
            return ImTextureRef(reinterpret_cast<ImTextureID>(nativeHandle));
        }
        else
        {
            return ImTextureRef(reinterpret_cast<uintptr_t>(nativeHandle));
        }
    }
}

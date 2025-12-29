#include "Gui/Panels/ContentBrowser/ContentEntry.h"
#include <array>
#include "Gui/Core/GuiCommon.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserState.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Utils/Editor/EditorUtils.h"


namespace BixEngine::Gui
{
    
    
    

    String ContentEntry::SelectionKey() const
    {
        if (path.empty())
            return name;

        std::filesystem::path key = path;
        if (IsScript())
            key /= name.View();

        return key.generic_string();
    }

    
    
    

    namespace
    {
        constexpr std::array<const char*, 6> kIcons{
            {
                "[DIR]", 
                "[FILE]", 
                "[SCRIPT]", 
                "[ACTOR]", 
                "[COMP]", 
                "[ATLAS]" 
            }
        };
    }

    const char* GetIcon(ContentType type)
    {
        const size_t index = static_cast<size_t>(type);
        return (index < kIcons.size()) ? kIcons[index] : kIcons[1];
    }

    
    
    

    int GetSortPriority(ContentType type)
    {
        switch (type)
        {
        case ContentType::Directory:
            return 0;

        case ContentType::ActorPrefab:
            return 1;

        case ContentType::ComponentPrefab:
            return 2;

        case ContentType::Script:
            return 3;

        case ContentType::SpriteAtlas:
            return 4;

        case ContentType::File:

        default:
            return 5;
        }
    }


    
    
    

    bool DrawEntryButton(const ContentEntry& entry, bool isSelected, ContentBrowserState& state, String& selectedEntry)
    {
        constexpr ImVec2 btnSize(Theme::ThumbnailSize, Theme::ThumbnailSize);
        const ImVec4 base = isSelected ? ImVec4(0.20f, 0.35f, 0.60f, 0.95f) : ImGui::GetStyleColorVec4(ImGuiCol_Button);

        Utils::ScopedColor button(ImGuiCol_Button, base);
        Utils::ScopedColor hover(ImGuiCol_ButtonHovered, Utils::AdjustColor(base, 0.08f));
        Utils::ScopedColor active(ImGuiCol_ButtonActive, Utils::AdjustColor(base, 0.14f));
        Utils::ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(12, 12));

        bool clicked = ImGui::Button(GetIcon(entry.type), btnSize);

        if (clicked)
        {
            if (entry.IsDirectory())
            {
                state.current = entry.path;
                state.cache.dirty = true;
                selectedEntry.Clear();
            }
            else
                selectedEntry = entry.SelectionKey();
        }

        if (Utils::IsItemDoubleClicked(ImGuiMouseButton_Left))
        {
            if (entry.IsDirectory())
            {
                state.current = entry.path;
                state.cache.dirty = true;
                selectedEntry.Clear();
            }
            else if (entry.IsScript())
            {
                std::vector<std::filesystem::path> files{};
                if (entry.HasHeader())
                    files.push_back(entry.headerPath);

                if (entry.HasSource())
                    files.push_back(entry.sourcePath);

                if (state.openScriptFilesCallback)
                    state.openScriptFilesCallback(files);

                for (auto& f : files)
                {
                    EditorUtils::Utilities::OpenFileInCodeEditor(f);
                }
            }
            else if (entry.IsPrefab() || entry.IsSpriteAtlas())
            {
                if (state.openAssetEditorCallback)
                    state.openAssetEditorCallback(entry.path);
            }
        }

        return clicked;
    }

    void DrawEntryTooltip(const ContentEntry& entry)
    {
        if (!ImGui::IsItemHovered(Theme::TooltipHoverFlags))
            return;

        if (ImGui::BeginTooltip())
        {
            ImGui::TextUnformatted(entry.name.c_str());
            ImGui::Separator();
            ImGui::Text("Path: %s", entry.path.generic_string().c_str());
            ImGui::EndTooltip();
        }
    }

    void DrawEntryLabel(const ContentEntry& entry, bool isSelected)
    {
        if (isSelected)
        {
            Utils::ScopedColor text(ImGuiCol_Text, ImVec4(0.95f, 0.85f, 0.40f, 1));
            ImGui::TextWrapped("%s", entry.name.c_str());
        }
        else
            ImGui::TextWrapped("%s", entry.name.c_str());
    }
}

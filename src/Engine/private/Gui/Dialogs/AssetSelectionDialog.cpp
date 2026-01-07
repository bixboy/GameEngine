#include "Gui/Dialogs/AssetSelectionDialog.h"
#include "Gui/Utils/ContentBrowserUtils.h"
#include "Utils/FileIO/FilesUtils.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Gui/Core/EditorPreferences.h"
#include "Utils/String/StringUtils.h"
#include <imgui.h>
#include <ranges>
#include <algorithm>

namespace BixEngine::Gui
{
    AssetSelectionDialog::AssetSelectionDialog()
    {
    }

    void AssetSelectionDialog::Open()
    {
        openRequested_ = true;
        cacheDirty_ = true;
        selectedPath_.clear();
        memset(searchBuffer_, 0, sizeof(searchBuffer_));
        selectionConfirmed_ = false;
    }

    void AssetSelectionDialog::Close()
    {
        isOpen_ = false;
        ImGui::CloseCurrentPopup();
    }

    bool AssetSelectionDialog::Render(const std::string& popupId, const std::vector<std::string>& allowedExtensions, std::string& outSelectedPath)
    {
        if (openRequested_)
        {
            ImGui::OpenPopup(popupId.c_str());
            openRequested_ = false;
            isOpen_ = true;
        }

        if (!isOpen_)
            return false;

        bool selectionMade = false;
        
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if (ImGui::BeginPopupModal(popupId.c_str(), &isOpen_))
        {
            // Header: Search
            GuiUtils::SearchInput("##SearchAsset", searchBuffer_, 128, "Search assets...");
            ImGui::Separator();

            // Body: Grid
            if (ImGui::BeginChild("AssetGrid", ImVec2(0, -40), true))
            {
                if (cacheDirty_)
                    RefreshCache(allowedExtensions);

                DrawGrid(allowedExtensions);
            }
            ImGui::EndChild();

            ImGui::Separator();

            // Footer: Buttons
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                Close();
            }

            ImGui::SameLine();

            bool hasSelection = !selectedPath_.empty();
            ImGui::BeginDisabled(!hasSelection);
            if (ImGui::Button("Select", ImVec2(120, 0)))
            {
                outSelectedPath = selectedPath_;
                selectionMade = true;
                Close();
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            ImGui::TextDisabled(selectedPath_.empty() ? "No selection" : selectedPath_.c_str());

            ImGui::EndPopup();
        }
        else
        {
             // Modal was closed (e.g. valid double click or internal close)
             if (selectionConfirmed_)
             {
                 outSelectedPath = selectedPath_;
                 selectionMade = true;
                 selectionConfirmed_ = false;
             }
             isOpen_ = false;
        }

        return selectionMade;
    }

    void AssetSelectionDialog::RefreshCache(const std::vector<std::string>& extensions)
    {
        cachedAssets_.clear();
        auto files = Utils::FileUtils::ScanDirectory(ContentBrowserUtils::GetContentRoot(), extensions);

        for (const auto& f : files)
        {
            AssetEntry entry;
            entry.path = f.generic_string();
            entry.name = f.filename().string();
            entry.extension = f.extension().string();
            cachedAssets_.push_back(entry);
        }

        cacheDirty_ = false;
    }

    void AssetSelectionDialog::DrawGrid(const std::vector<std::string>& extensions)
    {
        const auto& settings = EditorSettings::Get();
        const float cell = settings.ContentThumbnailSize + settings.ContentThumbnailPadding;
        const int cols = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / cell));

        if (ImGui::BeginTable("AssetEntries", cols, ImGuiTableFlags_SizingFixedFit))
        {
            for (const auto& entry : cachedAssets_)
            {
                if (searchBuffer_[0] != '\0')
                {
                   if (!StringUtils::Utilities::MatchesSearch(entry.name, searchBuffer_))
                       continue;
                }

                ImGui::TableNextColumn();
                
                bool isSelected = (selectedPath_ == entry.path);
                ImVec4 baseColor = isSelected ? ImVec4(0.2f,0.35f,0.6f,0.9f) : ImGui::GetStyleColorVec4(ImGuiCol_Button);
                
                GuiUtils::ScopedColor b(ImGuiCol_Button, baseColor);
                
                // Icon (using extension to guess type approx, or just generic file icon)
                // In a full implementation we'd reuse ContentBrowser logic for icons, 
                // for now let's use a generic function or check extension.
                // Assuming ContentBrowserUtils has a helper or we just use file icon.
                // `GetIcon` was available in ContentBrowserPanel but it's internal logic there via `GetIcon` helper.
                // We'll trust `ContentBrowserUtils::GetIconForExtension` if it exists update: it doesn't seem to be public.
                // We'll just use text for now or verify if we can access `DefaultEngineGui` icons?
                
                // For now, simple button with name. 
                // Improving: We can try to use `ImGui::Button` with size and text.
                
                if (ImGui::Button(entry.name.c_str(), { settings.ContentThumbnailSize, settings.ContentThumbnailSize }))
                {
                    selectedPath_ = entry.path;
                }
                
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    selectedPath_ = entry.path;
                    selectionConfirmed_ = true;
                    Close();
                }
                
                // Tooltip
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", entry.name.c_str());

                // Name label below
                ImGui::TextWrapped("%s", entry.name.c_str());
            }
            ImGui::EndTable();
        }
        
        // Handle double click logic if needed
        if (!isOpen_ && !selectedPath_.empty())
        {
            // If popup was closed by double click (via CloseCurrentPopup above?), we need a way to signal success.
            // Actually `CloseCurrentPopup` inside `BeginPopupModal` block just marks it to close at end of frame.
            // We need to detect this.
            // Let's change `DrawGrid` to return a boolean if double clicked.
        }
    }
}

#include "Gui/Dialogs/RenameEntryDialog.h"
#include "Debug/Logger.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserState.h"
#include "Utils/FileIO/FilesUtils.h"

using namespace BixEngine::Gui;
using namespace BixEngine::Gui::GuiUtils;
using namespace BixEngine::Utils;




RenameEntryDialog::RenameEntryDialog(ContentBrowserState& state, String& selectedEntry)
    : ModalDialog(state, selectedEntry, "ContentBrowserRenameEntry"), isScriptGroup_(false)
{
    renameBuffer_[0] = '\0';
    renameError_.clear();
}

void RenameEntryDialog::Open(const path& targetPath, const path& secondaryPath, bool isScriptGroup)
{
    targetPath_ = targetPath;
    secondaryPath_ = secondaryPath;
    isScriptGroup_ = isScriptGroup;

    std::string defaultName;
    if (isScriptGroup)
    {
        path basePath = !targetPath.empty() ? targetPath : secondaryPath;
        defaultName = basePath.stem().string();
    }
    else
    {
        defaultName = targetPath.filename().generic_string();
    }

    std::snprintf(renameBuffer_, IM_ARRAYSIZE(renameBuffer_), "%s", defaultName.c_str());
    renameError_.clear();
    ModalDialog::Open();
}




void RenameEntryDialog::ResetState()
{
    renameError_.clear();
    targetPath_.clear();
    secondaryPath_.clear();
    isScriptGroup_ = false;
}




void RenameEntryDialog::DrawContent()
{
    
    if (!targetPath_.empty())
    {
        String label = targetPath_.filename().generic_string().c_str();
        String prefix = isScriptGroup_ ? "Rename script: " : "Rename: ";
        DrawDescriptionText((prefix + label).c_str());
    }
    else
    {
        DrawDescriptionText("Rename the selected entry.");
    }

    
    bool renameTriggered = InputTextWithLabel("New name", renameBuffer_, IM_ARRAYSIZE(renameBuffer_), ImGuiInputTextFlags_EnterReturnsTrue, ImGui::IsWindowAppearing());

    if (!renameError_.IsEmpty())
        DrawErrorMessage(renameError_.Std());

    
    bool confirm = DrawConfirmButtons("Rename", "Cancel", []{},
        [&]
        {
            Close();
            ResetState();
        });

    if (confirm) renameTriggered = true;
    if (!renameTriggered) return;

    
    String newNameStr = renameBuffer_;
    if (newNameStr.IsEmpty())
    {
        FileUtils::LogAndStoreError(renameError_, "Name cannot be empty.", false);
        return;
    }

    if (newNameStr.Contains("/") || newNameStr.Contains("\\"))
    {
        FileUtils::LogAndStoreError(renameError_, "Name cannot contain path separators.", false);
        return;
    }

    if (!isScriptGroup_ && targetPath_.empty())
    {
        FileUtils::LogAndStoreError(renameError_, "No entry selected for rename.", false);
        return;
    }

    
    const bool success = isScriptGroup_ ? HandleScriptRename(newNameStr) : HandleSingleRename(newNameStr);

    if (success)
    {
        state_.cache.dirty = true;
        Close();
    }
}




bool RenameEntryDialog::HandleScriptRename(const String& newNameStr)
{
    path headerOld = targetPath_;
    path sourceOld = secondaryPath_;

    if (headerOld.empty() && sourceOld.empty())
        return FileUtils::LogAndStoreError(renameError_, "No entry selected for rename.", false), false;

    std::string newBaseName = newNameStr.Std();

    
    auto StripExt = [&](const path& p)
    {
        if (p.empty())
            return;

        std::string ext = p.extension().string();

        if (!ext.empty() && newBaseName.ends_with(ext))
            newBaseName.erase(newBaseName.size() - ext.size());
    };
    StripExt(headerOld);
    StripExt(sourceOld);

    if (newBaseName.empty())
        return FileUtils::LogAndStoreError(renameError_, "Name cannot be empty.", false), false;

    std::string currentBaseName = !headerOld.empty() ? headerOld.stem().string() : sourceOld.stem().string();
    if (newBaseName == currentBaseName)
        return true;

    
    path parentDir = !headerOld.empty() ? headerOld.parent_path() : sourceOld.parent_path();
    if (parentDir.empty())
        parentDir = state_.current;

    path newHeader = headerOld.empty() ? path{} : (parentDir / (newBaseName + headerOld.extension().string()));
    path newSource = sourceOld.empty() ? path{} : (parentDir / (newBaseName + sourceOld.extension().string()));

    if ((!newHeader.empty() && fs::exists(newHeader)) || (!newSource.empty() && fs::exists(newSource)))
        return FileUtils::LogAndStoreError(renameError_, "An entry with this name already exists.", false), false;

    
    std::vector<std::pair<path, path>> toRename;

    if (!headerOld.empty())
        toRename.emplace_back(headerOld, newHeader);

    if (!sourceOld.empty())
        toRename.emplace_back(sourceOld, newSource);

    std::vector<std::pair<path, path>> renamed;
    for (auto& [oldP, newP] : toRename)
    {
        std::error_code ec;
        fs::rename(oldP, newP, ec);
        if (ec)
        {
            String msg = "Unable to rename entry: " + String(ec.message().c_str());
            FileUtils::LogAndStoreError(renameError_, msg);

            for (auto it = renamed.rbegin(); it != renamed.rend(); ++it)
            {
                std::error_code rev;
                fs::rename(it->second, it->first, rev);
                if (rev)
                    LOG_ERROR("Rollback failed: " + String(rev.message().c_str()));
            }

            return false;
        }
        renamed.push_back({oldP, newP});
    }

    
    selectedEntry_ = (parentDir / newBaseName).generic_string().c_str();
    renameError_.clear();
    targetPath_ = newHeader;
    secondaryPath_ = newSource;
    isScriptGroup_ = false;

    return true;
}




bool RenameEntryDialog::HandleSingleRename(const String& newNameStr)
{
    path oldPath = targetPath_;
    if (oldPath.empty())
        return FileUtils::LogAndStoreError(renameError_, "Invalid target path.", false), false;

    std::string oldName = oldPath.filename().string();
    std::string newName = newNameStr.Std();

    if (oldName == newName)
        return true;

    path newPath = oldPath.parent_path() / newName;
    if (fs::exists(newPath))
        return FileUtils::LogAndStoreError(renameError_, "An entry with this name already exists.", false), false;

    if (!FileUtils::TryRename(oldPath, newPath, false, renameError_))
        return false;

    
    if (selectedEntry_.View() == oldPath.generic_string())
        selectedEntry_ = newPath.generic_string().c_str();

    targetPath_ = newPath;
    renameError_.clear();
    return true;
}

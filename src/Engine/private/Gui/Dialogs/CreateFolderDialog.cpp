#include "Gui/Dialogs/CreateFolderDialog.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserState.h"
#include "Utils/FileIO/FilesUtils.h"

using namespace BixEngine::Gui;





CreateFolderDialog::CreateFolderDialog(ContentBrowserState& state, String& selectedEntry) : ModalDialog(state, selectedEntry,
    "ContentBrowserCreateFolder")
{
    std::snprintf(folderName_, sizeof(folderName_), "%s", "NewFolder");
    folderError_.clear();
    targetDir_.clear();
}





void CreateFolderDialog::Open(const path& targetDirectory)
{
    std::snprintf(folderName_, sizeof(folderName_), "%s", "NewFolder");
    folderError_.clear();
    targetDir_ = targetDirectory;
    ModalDialog::Open();
}





void CreateFolderDialog::DrawContent()
{
    DrawHeader();
    DrawInputField();
    DrawError();
    DrawFooter();
}





void CreateFolderDialog::DrawHeader()
{
    const path baseDir = targetDir_.empty() ? state_.current : targetDir_;
    const path relPath = baseDir.lexically_relative(state_.root);
    std::string relString = relPath.generic_string();

    String description = "Create a new folder in: Content";
    if (!relString.empty() && relString != ".")
    {
        description += '/';
        description += relString.c_str();
    }

    GuiUtils::DrawDescriptionText(description.c_str());
}

void CreateFolderDialog::DrawInputField()
{
    GuiUtils::InputTextWithLabel("Folder name", folderName_, IM_ARRAYSIZE(folderName_), ImGuiInputTextFlags_EnterReturnsTrue, ImGui::IsWindowAppearing());
}

void CreateFolderDialog::DrawError()
{
    if (!folderError_.empty())
        GuiUtils::DrawErrorMessage(std::string(folderError_.View()));
}

void CreateFolderDialog::DrawFooter()
{
    const bool confirmed = GuiUtils::DrawConfirmButtons("Create", "Cancel", []{},
        [&]
        {
            Close();
            folderError_.clear();
            targetDir_.clear();
        });

    if (confirmed || ImGui::IsKeyPressed(ImGuiKey_Enter))
    {
        if (TryCreate())
            Close();
    }
}





bool CreateFolderDialog::TryCreate()
{
    String nameStr = String(folderName_).Trim();
    if (nameStr.empty())
        return Utils::FileUtils::LogAndStoreError(folderError_, "Folder name cannot be empty.", false);

    if (nameStr.Contains("/") || nameStr.Contains("\\"))
        return Utils::FileUtils::LogAndStoreError(folderError_, "Folder name cannot contain path separators.", false);

    const path baseDir = targetDir_.empty() ? state_.current : targetDir_;
    const path newFolderPath = baseDir / path(nameStr.View());

    if (fs::exists(newFolderPath))
        return Utils::FileUtils::LogAndStoreError(folderError_, "A folder with this name already exists.", false);

    if (!Utils::FileUtils::TryCreateDir(newFolderPath, folderError_))
        return false;

    
    selectedEntry_ = newFolderPath.generic_string().c_str();
    folderError_.clear();
    targetDir_.clear();
    state_.cache.dirty = true;

    return true;
}

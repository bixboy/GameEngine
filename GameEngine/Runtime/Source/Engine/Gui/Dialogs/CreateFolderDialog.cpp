#include "Engine/Gui/Dialogs/CreateFolderDialog.h"
#include "Engine/Gui/Utils/GuiHelpers.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserFileUtils.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserState.h"

using namespace BixEngine::Gui;
using namespace BixEngine::Gui::Utils;

// ─────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────

CreateFolderDialog::CreateFolderDialog(ContentBrowserState& state, String& selectedEntry)
    : ModalDialog(state, selectedEntry, "ContentBrowserCreateFolder")
{
    std::snprintf(folderName_, sizeof(folderName_), "%s", "NewFolder");
    folderError_.Clear();
    targetDir_.clear();
}

// ─────────────────────────────────────────────────────────────
// Open Dialog
// ─────────────────────────────────────────────────────────────

void CreateFolderDialog::Open(const path& targetDirectory)
{
    std::snprintf(folderName_, sizeof(folderName_), "%s", "NewFolder");
    folderError_.Clear();
    targetDir_ = targetDirectory;
    ModalDialog::Open();
}

// ─────────────────────────────────────────────────────────────
// Main Draw
// ─────────────────────────────────────────────────────────────

void CreateFolderDialog::DrawContent()
{
    DrawHeader();
    DrawInputField();
    DrawError();
    DrawFooter();
}

// ─────────────────────────────────────────────────────────────
// UI Elements 
// ─────────────────────────────────────────────────────────────

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

    DrawDescriptionText(description.c_str());
}

void CreateFolderDialog::DrawInputField()
{
    InputTextWithLabel("Folder name", folderName_, IM_ARRAYSIZE(folderName_), ImGuiInputTextFlags_EnterReturnsTrue, ImGui::IsWindowAppearing());
}

void CreateFolderDialog::DrawError()
{
    if (!folderError_.IsEmpty())
        DrawErrorMessage(std::string(folderError_.View()));
}

void CreateFolderDialog::DrawFooter()
{
    const bool confirmed = DrawConfirmButtons("Create", "Cancel", [] {}, [&]
    {
        Close();
        folderError_.Clear();
        targetDir_.clear();
    });

    if (confirmed || ImGui::IsKeyPressed(ImGuiKey_Enter))
    {
        if (TryCreate())
            Close();
    }
}

// ─────────────────────────────────────────────────────────────
// Folder Creation Logic
// ─────────────────────────────────────────────────────────────

bool CreateFolderDialog::TryCreate()
{
    String nameStr = TrimCopy(String(folderName_));
    if (nameStr.IsEmpty())
        return LogAndStoreError(folderError_, "Folder name cannot be empty.", false);

    if (ContainsPathSeparator(nameStr))
        return LogAndStoreError(folderError_, "Folder name cannot contain path separators.", false);

    const path baseDir = targetDir_.empty() ? state_.current : targetDir_;
    const path newFolderPath = baseDir / path(nameStr.View());

    if (fs::exists(newFolderPath))
        return LogAndStoreError(folderError_, "A folder with this name already exists.", false);

    if (!TryCreateDir(newFolderPath, folderError_))
        return false;

    // Success
    selectedEntry_ = newFolderPath.generic_string().c_str();
    folderError_.Clear();
    targetDir_.clear();
    state_.cache.dirty = true;

    return true;
}
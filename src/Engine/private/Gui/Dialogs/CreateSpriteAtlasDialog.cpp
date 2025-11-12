#include "Gui/Dialogs/CreateSpriteAtlasDialog.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Ressources/AtlasGenerator.h"
#include "Logger.h"
#include "Utils/FilesUtils.h"

using namespace BixEngine::Gui;
using namespace BixEngine::Gui::Utils;
using BixEngine::resources::AtlasGenerator;

// ─────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────

CreateSpriteAtlasDialog::CreateSpriteAtlasDialog(ContentBrowserState& state, String& selectedEntry)
    : ModalDialog(state, selectedEntry, "ContentBrowserCreateSpriteAtlas"), columns_(0), rows_(0), frameRate_(24.0f), loop_(true)
{
    atlasName_[0] = '\0';
    atlasError_.Clear();
    framesDir_.clear();
}

// ─────────────────────────────────────────────────────────────
// Dialog Entry Point
// ─────────────────────────────────────────────────────────────

void CreateSpriteAtlasDialog::Open(const path& sourcePath)
{
    atlasError_.Clear();

    // Determine base directory for frames
    framesDir_ = fs::is_directory(sourcePath) ? sourcePath : sourcePath.parent_path();

    // Default name proposal
    std::string defaultName = fs::is_directory(sourcePath)
                                  ? sourcePath.filename().string()
                                  : sourcePath.stem().string();

    if (defaultName.empty()) defaultName = "SpriteAtlas";

    std::snprintf(atlasName_, IM_ARRAYSIZE(atlasName_), "%s", defaultName.c_str());

    // Reset parameters
    columns_ = 0;
    rows_ = 0;
    frameRate_ = 24.0f;
    loop_ = true;

    ModalDialog::Open();
}

// ─────────────────────────────────────────────────────────────
// Rendering and Input
// ─────────────────────────────────────────────────────────────

void CreateSpriteAtlasDialog::DrawContent()
{
    DrawHeader();
    DrawInputFields();

    if (!atlasError_.IsEmpty())
        DrawErrorMessage(std::string(atlasError_.View()));

    if (!DrawConfirmButtons("Create", "Cancel", [](){},
        [&]
        {
            Close();
            atlasError_.Clear();
            framesDir_.clear();
        }))
        
        return;

    if (TryGenerateAtlas())
    {
        Close();
    }
}

// ─────────────────────────────────────────────────────────────
// UI Subsections
// ─────────────────────────────────────────────────────────────

void CreateSpriteAtlasDialog::DrawHeader()
{
    if (!framesDir_.empty())
    {
        String msg = "Generate a sprite atlas from images in folder: ";
        msg += framesDir_.filename().string().c_str();
        DrawDescriptionText(msg.c_str());
    }
    else
    {
        DrawDescriptionText("Generate a sprite atlas from PNG frames.");
    }
}

void CreateSpriteAtlasDialog::DrawInputFields()
{
    InputTextWithLabel("Atlas name (.atlas)", atlasName_, IM_ARRAYSIZE(atlasName_));

    ImGui::Text("Grid layout:");
    ImGui::SameLine();
    ImGui::PushItemWidth(80);
    ImGui::InputInt("Columns", &columns_);
    ImGui::SameLine();
    ImGui::InputInt("Rows", &rows_);
    ImGui::PopItemWidth();

    ImGui::InputFloat("Frame rate (fps)", &frameRate_, 0.0f, 0.0f, "%.1f");
    ImGui::Checkbox("Loop animation", &loop_);
}

// ─────────────────────────────────────────────────────────────
// Atlas Generation
// ─────────────────────────────────────────────────────────────

bool CreateSpriteAtlasDialog::TryGenerateAtlas()
{
    String nameStr = atlasName_;
    if (nameStr.IsEmpty())
    {
        FileUtils::LogAndStoreError(atlasError_, "Atlas name cannot be empty.", false);
        return false;
    }

    std::string baseName(nameStr.View().data(), nameStr.View().size());
    if (baseName.ends_with(".atlas"))
        baseName.erase(baseName.size() - 6);

    if (baseName.empty())
        baseName = "SpriteAtlas";

    path atlasPath = framesDir_ / (baseName + ".atlas");
    path imagePath = framesDir_ / (baseName + ".png");

    if (exists(atlasPath) || exists(imagePath))
    {
        FileUtils::LogAndStoreError(atlasError_, "An atlas or image with this name already exists.", false);
        return false;
    }

    if (!AtlasGenerator::GenerateAtlas(framesDir_, columns_, rows_, 0, 0, frameRate_, loop_))
    {
        FileUtils::LogAndStoreError(atlasError_, "Failed to generate atlas (check for PNG files with matching sizes).", false);
        return false;
    }

    RenameGeneratedAtlasIfNeeded(baseName);

    selectedEntry_ = (framesDir_ / (baseName + ".atlas")).generic_string().c_str();
    state_.cache.dirty = true;
    atlasError_.Clear();

    return true;
}

void CreateSpriteAtlasDialog::RenameGeneratedAtlasIfNeeded(const std::string& desiredBaseName)
{
    path expectedAtlas = framesDir_ / (desiredBaseName + ".atlas");

    path parentName = framesDir_.parent_path().filename();

    path defaultAtlas = framesDir_ / (
        parentName.empty()
            ? framesDir_.filename().string() + ".atlas"
            : parentName.string() + ".atlas"
    );

    if (defaultAtlas == expectedAtlas || !fs::exists(defaultAtlas))
        return;

    std::error_code err;
    fs::rename(defaultAtlas, expectedAtlas, err);
    if (err)
    {
        LOG_ERROR(String("Unable to rename atlas file: ") + err.message().c_str());
    }
}
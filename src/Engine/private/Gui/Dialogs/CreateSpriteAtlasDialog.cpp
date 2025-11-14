#include "Gui/Dialogs/CreateSpriteAtlasDialog.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Ressources/AtlasGenerator.h"
#include "Ressources/SpriteAtlasFactory.h"
#include "Logger.h"
#include "Utils/FilesUtils.h"

#include <algorithm>
#include <cstdio>
#include <cctype>
#include <optional>
#include <limits>
#include <cmath>

#include "Ressources/ResourceManager.h"
#include "Ressources/Texture.h"
#include "Ressources/SpriteAtlasUtils.h"

using namespace BixEngine::Gui;
using namespace BixEngine::Gui::Utils;
using BixEngine::resources::AtlasGenerator;
using BixEngine::resources::SpriteAtlasCreationParams;
using BixEngine::resources::SpriteAtlasFactory;

namespace
{
    namespace fs = std::filesystem;

    struct GridDetectionResult
    {
        int columns{0};
        int rows{0};
        int padding{0};
        int margin{0};
    };

    [[nodiscard]] int ExtractTrailingFrameCount(const std::string& stem)
    {
        if (stem.empty())
            return 0;

        size_t index = stem.size();
        while (index > 0 && std::isdigit(static_cast<unsigned char>(stem[index - 1])))
            --index;

        if (index == stem.size())
            return 0;

        if (index == 0)
            return 0;

        const char separator = stem[index - 1];
        if (separator != '_' && separator != '-' && separator != ' ')
            return 0;

        const std::string digits = stem.substr(index);
        try
        {
            const long long value = std::stoll(digits);
            if (value <= 0 || value > std::numeric_limits<int>::max())
                return 0;
            return static_cast<int>(value);
        }
        catch (...)
        {
            return 0;
        }
    }

    [[nodiscard]] std::optional<GridDetectionResult> DetectGridFromFrameCount(int frameCount, int texWidth,
                                                                              int texHeight)
    {
        if (frameCount <= 0 || texWidth <= 0 || texHeight <= 0)
            return std::nullopt;

        const float targetAspect = static_cast<float>(texWidth) / static_cast<float>(texHeight);

        struct Candidate
        {
            int columns;
            int rows;
            float score;
        };

        std::optional<Candidate> best;

        for (int rows = 1; rows <= frameCount; ++rows)
        {
            if (frameCount % rows != 0)
                continue;

            const int columns = frameCount / rows;
            if (columns <= 0)
                continue;

            if (texWidth % columns != 0 || texHeight % rows != 0)
                continue;

            const float aspect = static_cast<float>(columns) / static_cast<float>(rows);
            const float score = std::abs(aspect - targetAspect);

            if (!best.has_value() || score < best->score - 1e-4f ||
                (std::abs(score - best->score) <= 1e-4f && columns >= best->columns))
            {
                best = Candidate{columns, rows, score};
            }
        }

        if (!best.has_value())
            return std::nullopt;

        return GridDetectionResult{best->columns, best->rows, 0, 0};
    }

    [[nodiscard]] std::optional<GridDetectionResult> DetectGridFromAspect(int texWidth, int texHeight)
    {
        if (texWidth <= 0 || texHeight <= 0)
            return std::nullopt;

        if (texWidth >= texHeight && texHeight > 0 && texWidth % texHeight == 0)
            return GridDetectionResult{texWidth / texHeight, 1, 0, 0};

        if (texHeight > texWidth && texWidth > 0 && texHeight % texWidth == 0)
            return GridDetectionResult{1, texHeight / texWidth, 0, 0};

        return GridDetectionResult{1, 1, 0, 0};
    }
}

// ─────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────

CreateSpriteAtlasDialog::CreateSpriteAtlasDialog(ContentBrowserState& state, String& selectedEntry)
    : ModalDialog(state, selectedEntry, "ContentBrowserCreateSpriteAtlas"),
      columns_(0),
      rows_(0),
      frameRate_(24.0f),
      loop_(true),
      padding_(0),
      margin_(0)
{
    atlasName_[0] = '\0';
    texturePathBuffer_[0] = '\0';
    atlasError_.Clear();
    framesDir_.clear();
    texturePath_.clear();
    textureCandidates_.clear();
}

// ─────────────────────────────────────────────────────────────
// Dialog Entry Point
// ─────────────────────────────────────────────────────────────

void CreateSpriteAtlasDialog::Open(const path& sourcePath)
{
    atlasError_.Clear();

    // Determine base directory for frames / target output
    framesDir_ = fs::is_directory(sourcePath) ? sourcePath : sourcePath.parent_path();

    // Default name proposal
    std::string defaultName = fs::is_directory(sourcePath)
                                  ? sourcePath.filename().string()
                                  : sourcePath.stem().string();

    if (defaultName.empty())
        defaultName = "SpriteAtlas";

    std::snprintf(atlasName_, IM_ARRAYSIZE(atlasName_), "%s", defaultName.c_str());

    // Reset parameters
    columns_ = 0;
    rows_ = 0;
    padding_ = 0;
    margin_ = 0;
    frameRate_ = 24.0f;
    loop_ = true;

    texturePath_.clear();
    textureCandidates_.clear();
    texturePathBuffer_[0] = '\0';

    RefreshTextureCandidates();

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

    if (!DrawConfirmButtons("Create", "Cancel", []{},
            [&]
            {
                Close();
                atlasError_.Clear();
                framesDir_.clear();
                texturePath_.clear();
                textureCandidates_.clear();
                texturePathBuffer_[0] = '\0';
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
    if (!texturePath_.empty())
    {
        String msg = "Create a sprite atlas for texture: ";
        msg += texturePath_.filename().string().c_str();
        DrawDescriptionText(msg.c_str());
        return;
    }

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

    ImGui::InputInt("Padding", &padding_);
    ImGui::InputInt("Margin", &margin_);

    ImGui::InputFloat("Frame rate (fps)", &frameRate_, 0.0f, 0.0f, "%.1f");
    ImGui::Checkbox("Loop animation", &loop_);

    ImGui::Spacing();
    DrawTextureSelector();
}

void CreateSpriteAtlasDialog::DrawTextureSelector()
{
    DrawSeparatorText("Texture source");

    ImGui::Columns(2, nullptr, false);

    //
    // ───────────────────────────────────────────
    // LEFT COLUMN — PNG LIST
    // ───────────────────────────────────────────
    //
    ImGui::Text("PNG files:");
    ImGui::Separator();

    ImGui::BeginChild("PNG_LIST", ImVec2(0, 250), true);

    for (const auto& candidate : textureCandidates_)
    {
        const std::string display = GetDisplayName(candidate);
        const bool selected = (candidate == texturePath_);

        if (ImGui::Selectable(display.c_str(), selected))
        {
            SetTexturePath(candidate);
            atlasError_.Clear();
        }
    }

    if (textureCandidates_.empty())
        ImGui::TextDisabled("No PNG textures found.");

    ImGui::EndChild();

    if (ImGui::Button("Refresh"))
        RefreshTextureCandidates();

    ImGui::NextColumn();


    //
    // ───────────────────────────────────────────
    // RIGHT COLUMN — PREVIEW + PATH
    // ───────────────────────────────────────────
    //
    ImGui::Text("Selected texture:");
    ImGui::Separator();

    const bool edited = InputTextWithLabel("Path", texturePathBuffer_, IM_ARRAYSIZE(texturePathBuffer_));

    if (edited)
    {
        if (texturePathBuffer_[0] == '\0')
            texturePath_.clear();
        else
            texturePath_ = path(texturePathBuffer_);

        TryAutoConfigureFromTexture();
    }

    ImGui::Spacing();
    ImGui::Text("Preview:");

    // No texture selected
    if (texturePath_.empty() || !fs::exists(texturePath_))
    {
        ImGui::TextDisabled("No texture selected.");
        ImGui::Columns(1);
        return;
    }

    // Load texture resource
    auto tex = resources::ResourceManager::Get().Get<resources::Texture>(
        texturePath_.generic_string().c_str()
    );

    if (!tex)
    {
        ImGui::TextDisabled("Failed to load texture.");
        ImGui::Columns(1);
        return;
    }

    if (!tex->GetNativeHandle())
    {
        ImGui::TextDisabled("Texture invalid or not loaded.");
        ImGui::Columns(1);
        return;
    }

    //
    // Display preview (SDL_Texture*)
    //
    float previewW = 128.0f;
    float aspect = (float)tex->GetWidth() / (float)tex->GetHeight();
    float previewH = previewW / aspect;

    const ImTextureRef previewRef = Utils::ToTextureRef(tex->GetNativeHandle());
    if (previewRef.GetTexID() == ImTextureID_Invalid)
    {
        ImGui::TextDisabled("Unable to prepare texture preview.");
        ImGui::Columns(1);
        return;
    }

    ImGui::Image(previewRef, ImVec2(previewW, previewH));

    const ImVec2 imageMin = ImGui::GetItemRectMin();
    const ImVec2 imageMax = ImGui::GetItemRectMax();
    const float scaleX = (imageMax.x - imageMin.x) / static_cast<float>(tex->GetWidth());
    const float scaleY = (imageMax.y - imageMin.y) / static_cast<float>(tex->GetHeight());

    auto frames = resources::SpriteAtlasUtils::GenerateFrames(*tex, columns_, rows_, padding_, margin_);
    if (!frames.empty())
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        for (const auto& frame : frames)
        {
            const auto& rect = frame.uvRect;
            const ImVec2 min(imageMin.x + rect.x * scaleX, imageMin.y + rect.y * scaleY);
            const ImVec2 max(imageMin.x + (rect.x + rect.width) * scaleX,
                             imageMin.y + (rect.y + rect.height) * scaleY);
            drawList->AddRect(min, max, IM_COL32(255, 255, 255, 160));
        }
    }
    else
    {
        ImGui::TextDisabled("Unable to compute frame slices with the current layout.");
    }

    ImGui::Text("Size: %d x %d", tex->GetWidth(), tex->GetHeight());
    ImGui::Text("File: %s", texturePath_.filename().string().c_str());

    ImGui::Columns(1);
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

    const path resolvedTexture = ResolveTexturePath();
    const bool useExistingTexture = !resolvedTexture.empty();

    path atlasDirectory = useExistingTexture ? resolvedTexture.parent_path() : framesDir_;

    if (atlasDirectory.empty())
    {
        FileUtils::LogAndStoreError(atlasError_, "Unable to determine a target directory for the atlas.", false);
        return false;
    }

    path atlasPath = atlasDirectory / (baseName + ".atlas");
    if (fs::exists(atlasPath))
    {
        FileUtils::LogAndStoreError(atlasError_, "An atlas with this name already exists.", false);
        return false;
    }

    if (useExistingTexture)
    {
        SpriteAtlasCreationParams params{};
        params.texturePath = resolvedTexture;
        params.columns = std::max(columns_, 1);
        params.rows = std::max(rows_, 1);
        params.padding = std::max(padding_, 0);
        params.margin = std::max(margin_, 0);

        if (!SpriteAtlasFactory::CreateAtlasFile(atlasPath, params, atlasError_))
            return false;
    }
    else
    {
        if (framesDir_.empty() || !fs::exists(framesDir_))
        {
            FileUtils::LogAndStoreError(atlasError_, "Select a directory containing PNG frames before generating an atlas.", false);
            return false;
        }

        const path imagePath = framesDir_ / (baseName + ".png");
        if (fs::exists(imagePath))
        {
            FileUtils::LogAndStoreError(atlasError_, "An image with this name already exists in the target directory.", false);
            return false;
        }

        const int padding = std::max(padding_, 0);
        const int margin = std::max(margin_, 0);

        if (!AtlasGenerator::GenerateAtlas(framesDir_, columns_, rows_, padding, margin, frameRate_, loop_))
        {
            FileUtils::LogAndStoreError(atlasError_,
                "Failed to generate atlas (ensure PNG frames share the same resolution).",
                false);
            return false;
        }

        RenameGeneratedAtlasIfNeeded(baseName);
        atlasPath = framesDir_ / (baseName + ".atlas");
    }

    selectedEntry_ = atlasPath.generic_string().c_str();
    state_.cache.dirty = true;
    atlasError_.Clear();

    return true;
}

void CreateSpriteAtlasDialog::RenameGeneratedAtlasIfNeeded(const std::string& desiredBaseName)
{
    path expectedAtlas = framesDir_ / (desiredBaseName + ".atlas");

    path parentName = framesDir_.parent_path().filename();

    path defaultAtlas = framesDir_ /
        (parentName.empty()
            ? framesDir_.filename().string() + ".atlas"
            : parentName.string() + ".atlas");

    if (defaultAtlas == expectedAtlas || !fs::exists(defaultAtlas))
        return;

    std::error_code err;
    fs::rename(defaultAtlas, expectedAtlas, err);
    if (err)
    {
        LOG_ERROR(String("Unable to rename atlas file: ") + err.message().c_str());
    }
}

void CreateSpriteAtlasDialog::RefreshTextureCandidates()
{
    textureCandidates_.clear();

    if (framesDir_.empty() || !fs::exists(framesDir_))
        return;

    std::error_code iterError;
    for (const auto& entry : fs::directory_iterator(framesDir_, iterError))
    {
        if (iterError)
            break;

        if (!entry.is_regular_file())
            continue;

        const auto ext = entry.path().extension().string();
        if (ext == ".png" || ext == ".PNG")
        {
            std::error_code canonicalError;
            path candidate = fs::weakly_canonical(entry.path(), canonicalError);
            textureCandidates_.push_back(canonicalError ? entry.path() : candidate);
        }
    }

    std::sort(textureCandidates_.begin(), textureCandidates_.end(), [](const path& a, const path& b)
    {
        return FileUtils::CaseInsensitiveLess(a.filename().string(), b.filename().string());
    });

    if (texturePath_.empty() && !textureCandidates_.empty())
        SetTexturePath(textureCandidates_.front());
}

void CreateSpriteAtlasDialog::SetTexturePath(const path& newPath)
{
    if (newPath.empty())
    {
        texturePath_.clear();
        texturePathBuffer_[0] = '\0';
        return;
    }

    std::error_code canonicalError;
    texturePath_ = fs::weakly_canonical(newPath, canonicalError);
    if (canonicalError)
        texturePath_ = newPath;

    const std::string display = GetDisplayName(texturePath_);
    std::snprintf(texturePathBuffer_, IM_ARRAYSIZE(texturePathBuffer_), "%s", display.c_str());

    TryAutoConfigureFromTexture();
}

void CreateSpriteAtlasDialog::TryAutoConfigureFromTexture()
{
    if (texturePath_.empty())
        return;

    if (!fs::exists(texturePath_))
        return;

    auto texture = resources::ResourceManager::Get().Get<resources::Texture>(
        texturePath_.generic_string().c_str());

    if (!texture)
        return;

    const int texWidth = texture->GetWidth();
    const int texHeight = texture->GetHeight();
    if (texWidth <= 0 || texHeight <= 0)
        return;

    std::optional<GridDetectionResult> detected;

    const int frameCount = ExtractTrailingFrameCount(texturePath_.stem().string());
    if (frameCount > 0)
        detected = DetectGridFromFrameCount(frameCount, texWidth, texHeight);

    if (!detected.has_value())
        detected = DetectGridFromAspect(texWidth, texHeight);

    if (!detected.has_value())
        return;

    columns_ = std::max(1, detected->columns);
    rows_ = std::max(1, detected->rows);
    padding_ = std::max(0, detected->padding);
    margin_ = std::max(0, detected->margin);
}

path CreateSpriteAtlasDialog::ResolveTexturePath() const
{
    if (!texturePath_.empty())
    {
        std::error_code canonicalError;
        path canonical = fs::weakly_canonical(texturePath_, canonicalError);
        return canonicalError ? texturePath_ : canonical;
    }

    if (texturePathBuffer_[0] == '\0')
        return {};

    path typed(texturePathBuffer_);
    if (typed.is_absolute())
        return typed;

    if (!framesDir_.empty())
    {
        path candidate = framesDir_ / typed;
        if (fs::exists(candidate))
            return candidate;
    }

    if (!state_.root.empty())
    {
        path candidate = state_.root / typed;
        if (fs::exists(candidate))
            return candidate;
    }

    return typed;
}

std::string CreateSpriteAtlasDialog::GetDisplayName(const path& value) const
{
    if (value.empty())
        return {};

    if (!state_.root.empty())
    {
        std::error_code relativeError;
        path relative = fs::relative(value, state_.root, relativeError);
        
        if (!relativeError && !relative.empty() && relative.native().rfind(L"..", 0) != 0)
            return relative.generic_string();
    }

    return value.generic_string();
}

path CreateSpriteAtlasDialog::GetSelectedContentPath() const
{
    if (selectedEntry_.IsEmpty())
        return {};

    path candidate(selectedEntry_.c_str());

    if (candidate.is_relative() && !state_.root.empty())
    {
        path resolved = state_.root / candidate;
        if (fs::exists(resolved))
            return resolved;
    }

    if (fs::exists(candidate))
        return candidate;

    return {};
}

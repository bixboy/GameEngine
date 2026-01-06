#include "Gui/Dialogs/CreateSpriteAtlasDialog.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Ressources/Atlas/AtlasGenerator.h"
#include "Ressources/Atlas/SpriteAtlasFactory.h"
#include "Ressources/Atlas/SpriteAtlasUtils.h"
#include "Ressources/Core/ResourceManager.h"
#include "Ressources/RessourcesClass/Texture.h"
#include "Utils/FileIO/FilesUtils.h"
#include <algorithm>

using namespace BixEngine::Gui;
using namespace BixEngine::Gui::GuiUtils;
using namespace BixEngine::Resources;
using namespace BixEngine::Utils;





CreateSpriteAtlasDialog::CreateSpriteAtlasDialog(ContentBrowserState& s, String& selected) : ModalDialog(s, selected, "ContentBrowserCreateSpriteAtlas")
{
    atlasName_[0] = texturePathBuffer_[0] = '\0';
    columns_ = rows_ = 0.0f;
    padding_ = margin_ = 0;
    frameRate_ = 24.f;
    loop_ = true;
}




void CreateSpriteAtlasDialog::Open(const path& src)
{
    atlasError_.clear();

    framesDir_ = fs::is_directory(src) ? src : src.parent_path();
    std::string name = fs::is_directory(src) ? src.filename().string() : src.stem().string();
    if (name.empty()) name = "SpriteAtlas";

    printf(atlasName_, IM_ARRAYSIZE(atlasName_), "%s", name.c_str());

    columns_ = rows_ = 0.0f;
    padding_ = margin_ = 0;
    frameRate_ = 24.f;
    loop_ = true;
    textureCandidates_.clear();
    texturePath_.clear();
    texturePathBuffer_[0] = '\0';

    RefreshTextureCandidates();
    ModalDialog::Open();
}

void CreateSpriteAtlasDialog::SetTexturePath(const path& newPath)
{
    if (newPath.empty())
    {
        texturePath_.clear();
        texturePathBuffer_[0] = '\0';
        return;
    }

    std::error_code err;
    texturePath_ = fs::weakly_canonical(newPath, err);
    if (err) texturePath_ = newPath;

    std::string display = GetDisplayName(texturePath_);
    std::snprintf(texturePathBuffer_, IM_ARRAYSIZE(texturePathBuffer_), "%s", display.c_str());

    TryAutoConfigureFromTexture();
}





void CreateSpriteAtlasDialog::DrawContent()
{
    DrawHeader();
    DrawInputFields();

        DrawErrorMessage(atlasError_.Std());

    if (!DrawConfirmButtons("Create","Cancel",
        []{},
        [&]{ Close(); atlasError_.clear(); texturePath_.clear(); textureCandidates_.clear(); }))
        return;

    if (TryGenerateAtlas())
        Close();
}




void CreateSpriteAtlasDialog::DrawHeader()
{
    if (!texturePath_.empty())
    {
        DrawDescriptionText(("Create a sprite atlas for: " + texturePath_.filename().string()).c_str());
        return;
    }

    if (!framesDir_.empty())
    {
        DrawDescriptionText(("Generate atlas from folder: " + framesDir_.filename().string()).c_str());
        return;
    }

    DrawDescriptionText("Generate a sprite atlas from PNG frames.");
}




void CreateSpriteAtlasDialog::DrawInputFields()
{
    InputTextWithLabel("Atlas name (.atlas)", atlasName_, IM_ARRAYSIZE(atlasName_));

    ImGui::Text("Grid:");
    ImGui::SameLine();
    ImGui::PushItemWidth(80);
    ImGui::InputFloat("Columns", &columns_, 1.0f, 1.0f, "%.1f");
    ImGui::SameLine();
    ImGui::InputFloat("Rows", &rows_, 1.0f, 1.0f, "%.1f");
    ImGui::PopItemWidth();

    ImGui::InputInt("Padding", &padding_);
    ImGui::InputInt("Margin", &margin_);

    ImGui::InputFloat("Frame rate (fps)", &frameRate_);
    ImGui::Checkbox("Loop animation", &loop_);

    ImGui::Spacing();
    DrawTextureSelector();
}




void CreateSpriteAtlasDialog::DrawTextureSelector()
{
    DrawSeparatorText("Texture source");
    ImGui::Columns(2);

    
    ImGui::Text("PNG files:");
    ImGui::Separator();
    ImGui::BeginChild("PNG_LIST", ImVec2(0,250), true);

    for (auto& p : textureCandidates_)
    {
        bool selected = (p == texturePath_);
        if (ImGui::Selectable(GetDisplayName(p).c_str(), selected))
        {
            SetTexturePath(p);
            atlasError_.clear();
        }
    }

    if (textureCandidates_.empty())
        ImGui::TextDisabled("No PNG textures found.");

    ImGui::EndChild();
    if (ImGui::Button("Refresh")) RefreshTextureCandidates();
    ImGui::NextColumn();

    
    ImGui::Text("Selected texture:");
    ImGui::Separator();

    bool edited = InputTextWithLabel("Path", texturePathBuffer_, IM_ARRAYSIZE(texturePathBuffer_));
    if (edited)
    {
        texturePath_ = (texturePathBuffer_[0] ? path(texturePathBuffer_) : path());
        TryAutoConfigureFromTexture();
    }

    ImGui::Spacing();
    ImGui::Text("Preview:");

    if (texturePath_.empty() || !fs::exists(texturePath_))
    {
        ImGui::TextDisabled("No texture selected.");
        ImGui::Columns(1);
        return;
    }

    auto tex = ResourceManager::Get().Get<Texture>(texturePath_.string().c_str());
    if (!tex || !tex->GetNativeHandle())
    {
        ImGui::TextDisabled("Failed to load texture.");
        ImGui::Columns(1);
        return;
    }

    float previewW = 128.f;
    float aspect = (float)tex->GetWidth() / tex->GetHeight();
    float previewH = previewW / aspect;

    ImTextureRef ref = GuiUtils::ToTextureRef(tex->GetNativeHandle());
    if (ref.GetTexID() == ImTextureID_Invalid)
    {
        ImGui::TextDisabled("Preview unavailable.");
        ImGui::Columns(1);
        return;
    }

    ImGui::Image(ref, ImVec2(previewW, previewH));

    
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    float sx = (max.x-min.x) / tex->GetWidth();
    float sy = (max.y-min.y) / tex->GetHeight();

    auto frames = SpriteAtlasUtils::GenerateFrames(*tex, (int)columns_, (int)rows_, padding_, margin_);
    if (!frames.empty())
    {
        auto* dl = ImGui::GetWindowDrawList();
        for (auto& f : frames)
        {
            auto& r=f.uvRect;
            dl->AddRect(
                {min.x+r.x*sx, min.y+r.y*sy},
                {min.x+(r.x+r.width)*sx, min.y+(r.y+r.height)*sy},
                IM_COL32(255,255,255,180)
            );
        }
    }
    else ImGui::TextDisabled("Invalid grid slicing.");

    ImGui::Text("Size: %d x %d", tex->GetWidth(), tex->GetHeight());
    ImGui::Text("File: %s", texturePath_.filename().string().c_str());

    ImGui::Columns(1);
}




void CreateSpriteAtlasDialog::TryAutoConfigureFromTexture()
{
    if (texturePath_.empty() || !fs::exists(texturePath_))
        return;

    auto tex = ResourceManager::Get().Get<Texture>(texturePath_.string().c_str());
    if (!tex)
        return;

    int detectedCols = 0;
    int detectedRows = 0;

    if (!SpriteAtlasUtils::AutoDetectGrid(texturePath_, detectedCols, detectedRows))
        return;

    columns_ = std::max(1.0f, (float)detectedCols);
    rows_ = std::max(1.0f, (float)detectedRows);

    padding_ = 0;
    margin_  = 0;
}





bool CreateSpriteAtlasDialog::TryGenerateAtlas()
{
    if (atlasName_[0]=='\0')
        return FileUtils::LogAndStoreError(atlasError_,"Atlas name cannot be empty.",false), false;

    std::string base(atlasName_);
    if (base.ends_with(".atlas"))
        base.erase(base.size()-6);
    
    if (base.empty())
        base="SpriteAtlas";

    path tex = ResolveTexturePath();
    bool usingTexture = !tex.empty();

    path outDir = usingTexture ? tex.parent_path() : framesDir_;
    if (outDir.empty())
        return FileUtils::LogAndStoreError(atlasError_,"Invalid output directory.",false), false;

    path out = outDir / (base+".atlas");
    if (fs::exists(out))
        return FileUtils::LogAndStoreError(atlasError_,"Atlas already exists.",false), false;

    if (usingTexture)
    {
        SpriteAtlasCreationParams p;
        p.texturePath = tex;
        p.columns = std::max(1, (int)columns_);
        p.rows    = std::max(1, (int)rows_);
        p.padding = std::max(0, padding_);
        p.margin  = std::max(0, margin_);

        if (!SpriteAtlasFactory::CreateAtlasFile(out,p,atlasError_))
            return false;
    }
    else
    {
        if (framesDir_.empty() || !fs::exists(framesDir_))
            return FileUtils::LogAndStoreError(atlasError_,"Select a valid PNG folder.",false), false;

        
        int iCols = (int)std::max(1.0f, std::ceil(columns_));
        int iRows = (int)std::max(1.0f, std::ceil(rows_));

        if (!AtlasGenerator::GenerateAtlas(framesDir_,iCols,iRows,padding_,margin_,frameRate_,loop_))
            return FileUtils::LogAndStoreError(atlasError_,"Failed to generate atlas.",false), false;

        path autoOut = framesDir_ / (base+".atlas");
        if (fs::exists(autoOut))
            out = autoOut;
    }

    selectedEntry_ = out.generic_string().c_str();
    state_.cache.dirty = true;
    atlasError_.clear();
    return true;
}




void CreateSpriteAtlasDialog::RefreshTextureCandidates()
{
    textureCandidates_.clear();
    if (framesDir_.empty() || !fs::exists(framesDir_))
        return;

    for (auto& e : fs::directory_iterator(framesDir_))
    {
        if (!e.is_regular_file())
            continue;
        
        auto ext = e.path().extension().string();
        if (ext==".png" || ext==".PNG")
            textureCandidates_.push_back(e.path());
    }

    std::sort(textureCandidates_.begin(),textureCandidates_.end(),
        [](auto&a,auto&b){ return FileUtils::CaseInsensitiveLess(a.filename().string(),b.filename().string()); });

    if (texturePath_.empty() && !textureCandidates_.empty())
        SetTexturePath(textureCandidates_.front());
}

std::string CreateSpriteAtlasDialog::GetDisplayName(const path& p) const
{
    if (p.empty()) return {};
    if (state_.root.empty()) return p.generic_string();

    std::error_code ec;
    path rel = fs::relative(p,state_.root,ec);
    return (!ec && !rel.empty()) ? rel.generic_string() : p.generic_string();
}

path CreateSpriteAtlasDialog::ResolveTexturePath() const
{
    if (!texturePath_.empty())
        return texturePath_;
    
    if (texturePathBuffer_[0]=='\0')
        return {};

    path typed(texturePathBuffer_);
    if (typed.is_absolute())
        return typed;

    if (!framesDir_.empty())
    {
        path c = framesDir_/typed;
        if (fs::exists(c))
            return c;
    }

    if (!state_.root.empty())
    {
        path c = state_.root/typed;
        if (fs::exists(c))
            return c;
    }

    return typed;
}

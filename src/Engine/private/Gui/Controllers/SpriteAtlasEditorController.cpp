#include "Gui/Controllers/SpriteAtlasEditorController.h"
#include <algorithm>
#include <cstdio>
#include "Debug/Logger.h"
#include "Gui/Panels/GuiPanel.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Ressources/Atlas/SpriteAtlasFactory.h"
#include "Ressources/RessourcesClass/Texture.h"
#include "imgui.h"
#include "Gui/Core/EditorPreferences.h"
#include "Gui/Core/GuiTheme.h"
#include "Gui/Controllers/BaseAssetEditorController.h"


namespace BixEngine::Gui
{
    namespace
    {
        String BuildDisplayName(const std::filesystem::path& path)
        {
            if (path.empty())
                return "Sprite Atlas";

            const auto filename = path.filename().generic_string();
            if (!filename.empty())
                return filename;

            const auto stem = path.stem().generic_string();
            if (!stem.empty())
                return stem;

            return String(path.generic_string());
        }

        float ComputeCellDimension(float totalSize, float count, int padding, int margin)
        {
            if (count <= 0.001f)
                return 0.0f;
            const float totalPadding = std::max(0.0f, count - 1.0f) * padding;
            const float totalMargins = static_cast<float>(margin * 2);
            return (totalSize - totalPadding - totalMargins) / count;
        }
    }

    SpriteAtlasEditorController::SpriteAtlasEditorController(std::shared_ptr<SharedState> sharedState) : BaseAssetEditorController(std::move(sharedState),
        {
            .titlePrefix = "Sprite Atlas", .dockRegion = DockSpaceRegion::Center,
            .stableIdSuffix = "Atlas"
        }){}

    std::shared_ptr<SpriteAtlasEditorController::SharedState> SpriteAtlasEditorController::CreateSharedState(const std::filesystem::path& atlasPath,
        String stableIdRoot, std::function<void()> onCloseRequest)
    {
        auto state = std::make_shared<SharedState>();
        state->assetPath = atlasPath;
        state->atlasPath = atlasPath;
        state->stableIdRoot = std::move(stableIdRoot);
        state->assetDisplayName = BuildDisplayName(atlasPath);
        state->assetTypeLabel = "Sprite Atlas";
        state->onCloseRequest = std::move(onCloseRequest);

        resources::SpriteAtlasDefinition definition;
        std::vector<resources::SpriteAnimationDefinition> animations;
        if (!resources::SpriteAtlasUtils::ParseAtlasFile(String(atlasPath.generic_string().c_str()), definition,
                                                         animations))
        {
            state->error = "Failed to parse atlas file.";
        }
        else
        {
            state->definition = definition;
            state->animations = std::move(animations);
            const float safeColumns = std::max(0.0f, definition.columns);
            const float safeRows = std::max(0.0f, definition.rows);
            // Size is total integer cells required
            state->frameSelection.assign(static_cast<size_t>(std::ceil(safeColumns) * std::ceil(safeRows)), false);
            if (!state->animations.empty())
            {
                state->activeAnimation = 0;
                state->previewAnimationIndex = 0;
                state->previewPlaying = true;
            }

            const std::filesystem::path directory = atlasPath.parent_path();
            if (!definition.texturePath.IsEmpty())
            {
                state->textureAbsolutePath = directory / definition.texturePath.View();
            }
        }

        return state;
    }

    void SpriteAtlasEditorController::DrawPanelContents(GuiPanel&)
    {
        auto baseState = GetSharedState();
        auto state = std::static_pointer_cast<SharedState>(baseState);
        if (!state)
        {
            Utils::DrawEmptyStateMessage("No sprite atlas selected.");
            return;
        }

        // ────────────── Global error ──────────────
        if (!state->error.IsEmpty())
            Utils::DrawErrorMessage(std::string(state->error.View()));

        // ────────────── Atlas section ──────────────
        ImGui::TextUnformatted("🧩 Sprite Atlas Overview");
        ImGui::Separator();

        DrawAtlasPreview(*state);

        // Affiche dimensions et paramètres du layout
        ImGui::Spacing();
        ImGui::Text("Texture: %s", state->textureAbsolutePath.filename().string().c_str());
        ImGui::Text("Grid: %.2f cols × %.2f rows", state->definition.columns, state->definition.rows);
        ImGui::Text("Margin: %d  |  Padding: %d", state->definition.margin, state->definition.padding);
        ImGui::Separator();

        // ────────────── Animation section ──────────────
        ImGui::TextUnformatted("🎞️ Animations");
        ImGui::Separator();
        DrawAnimationSection(*state);

        // ────────────── Save section ──────────────
        ImGui::Spacing();
        ImGui::Separator();
        DrawSaveSection(*state);
    }


    void SpriteAtlasEditorController::OnSaveRequested()
    {
        auto baseState = GetSharedState();
        auto state = std::static_pointer_cast<SharedState>(baseState);
        if (!state)
            return;

        (void)SaveAtlas(*state);
    }

    void SpriteAtlasEditorController::DrawAtlasPreview(SharedState& state)
    {
        EnsureSelectionSize(state);
        RefreshTexture(state);

        if (!state.texture)
        {
            Utils::DrawEmptyStateMessage("Texture preview unavailable.");
            return;
        }

        const float texW = static_cast<float>(state.texture->GetWidth());
        const float texH = static_cast<float>(state.texture->GetHeight());
        if (texW <= 0.0f || texH <= 0.0f)
        {
            Utils::DrawEmptyStateMessage("Texture has invalid dimensions.");
            return;
        }

        const ImVec2 available = ImGui::GetContentRegionAvail();
        const float aspect = texH / texW;
        float dispW = available.x;
        float dispH = dispW * aspect;
        if (dispH > available.y * 0.5f && available.y > 0)
        {
            dispH = available.y * 0.5f;
            dispW = dispH / aspect;
        }

        const ImVec2 imgSize(dispW, dispH);
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const ImTextureRef previewRef = Utils::ToTextureRef(state.texture->GetNativeHandle());
        if (previewRef.GetTexID() == ImTextureID_Invalid)
        {
            Utils::DrawEmptyStateMessage("Texture preview unavailable.");
            return;
        }

        ImGui::Image(previewRef, imgSize);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        const float scaleX = dispW / texW;
        const float scaleY = dispH / texH;

        const float cols = std::max(1.0f, state.definition.columns);
        const float rows = std::max(1.0f, state.definition.rows);
        const float cellW = ComputeCellDimension(texW, cols, state.definition.padding, state.definition.margin);
        const float cellH = ComputeCellDimension(texH, rows, state.definition.padding, state.definition.margin);

        int hovered = -1;
        
        int iCols = (int)std::ceil(cols);
        int iRows = (int)std::ceil(rows);
        
        for (int y = 0; y < iRows; ++y)
        {
            for (int x = 0; x < iCols; ++x)
            {
                const float x0 = (state.definition.margin + x * (cellW + state.definition.padding));
                const float y0 = (state.definition.margin + y * (cellH + state.definition.padding));
                const float x1 = x0 + cellW;
                const float y1 = y0 + cellH;

                ImVec2 min(origin.x + x0 * scaleX, origin.y + y0 * scaleY);
                ImVec2 max(origin.x + x1 * scaleX, origin.y + y1 * scaleY);

                const int idx = y * iCols + x;
                const bool sel = idx < static_cast<int>(state.frameSelection.size()) && state.frameSelection[idx];

                const auto& settings = EditorSettings::Get();
                const ImU32 accentColor = ImGui::ColorConvertFloat4ToU32(settings.ThemeAccentColor);
                const ImU32 accentTransparent = (accentColor & 0x00FFFFFF) | 0x46000000; // Alpha 70
                const ImU32 hoverColor = IM_COL32(255, 255, 255, 25);
                const ImU32 gridColor = IM_COL32(255, 255, 255, 90);

                // surbrillance sélection / hover
                if (sel)
                    draw->AddRectFilled(min, max, accentTransparent);
                else if (ImGui::IsMouseHoveringRect(min, max))
                    draw->AddRectFilled(min, max, hoverColor);

                draw->AddRect(min, max, gridColor);

                if (ImGui::IsMouseHoveringRect(min, max))
                    hovered = idx;
            }
        }

        // Gestion clic
        if (hovered >= 0 && ImGui::IsItemHovered())
        {
            const bool append = ImGui::GetIO().KeyShift;
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                ToggleFrameSelection(state, hovered, append);

            // infobulle frame
            const int col = hovered % iCols;
            const int row = hovered / iCols;
            ImGui::BeginTooltip();
            ImGui::Text("Frame %d (%d,%d)", hovered, col, row);
            ImGui::EndTooltip();
        }
    }


    void SpriteAtlasEditorController::DrawAnimationSection(SharedState& state)
    {
        if (ImGui::Button("➕ Add Animation"))
        {
            resources::SpriteAnimationDefinition anim;
            anim.name = String("Animation ") + String(std::to_string(state.animations.size() + 1));
            anim.frameRate = 8.f;
            anim.loop = true;
            state.animations.push_back(anim);
            state.activeAnimation = static_cast<int>(state.animations.size()) - 1;
            state.dirty = true;
        }

        ImGui::SameLine();
        bool valid = state.activeAnimation >= 0 && state.activeAnimation < static_cast<int>(state.animations.size());
        ImGui::BeginDisabled(!valid);
        if (ImGui::Button("❌ Delete"))
            RemoveSelectedAnimation(state, state.activeAnimation);
        ImGui::EndDisabled();

        ImGui::Separator();

        if (ImGui::BeginTable("AnimTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("List", ImGuiTableColumnFlags_WidthStretch, 0.3f);
            ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch, 0.7f);
            ImGui::TableNextColumn();

            // Liste animations
            if (ImGui::BeginChild("AnimList", ImVec2(0, 200), true))
            {
                for (int i = 0; i < static_cast<int>(state.animations.size()); ++i)
                {
                    bool sel = (i == state.activeAnimation);
                    const std::string label = state.animations[i].name.IsEmpty()
                                                  ? ("Animation " + std::to_string(i + 1))
                                                  : std::string(state.animations[i].name.View());
                    if (ImGui::Selectable(label.c_str(), sel))
                    {
                        state.activeAnimation = i;
                        state.previewAnimationIndex = -1;
                        state.previewFrame = 0;
                    }
                }
            }
            ImGui::EndChild();

            // Détails
            ImGui::TableNextColumn();
            if (!valid)
            {
                Utils::DrawEmptyStateMessage("Select an animation to edit.");
            }
            else
            {
                auto& anim = state.animations[state.activeAnimation];

                char buffer[128]{};
                std::snprintf(buffer, sizeof(buffer), "%s", anim.name.IsEmpty() ? "" : anim.name.Std().c_str());
                if (ImGui::InputText("Name", buffer, sizeof(buffer)))
                {
                    anim.name = buffer;
                    state.dirty = true;
                }

                if (ImGui::DragFloat("Frame Rate", &anim.frameRate, 0.1f, 0.f, 120.f, "%.2f"))
                    state.dirty = true;
                if (ImGui::Checkbox("Loop", &anim.loop))
                    state.dirty = true;

                ImGui::Separator();

                if (ImGui::Button("🎯 Use selected frames"))
                {
                    AssignSelectionToAnimation(state, anim);
                    state.dirty = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("🧹 Clear"))
                {
                    anim.frames.clear();
                    state.dirty = true;
                }

                DrawAnimationPreview(state, anim);
            }

            ImGui::EndTable();
        }
    }


    void SpriteAtlasEditorController::DrawAnimationPreview(SharedState& state,
                                                           resources::SpriteAnimationDefinition& anim)
    {
        ImGui::Spacing();
        ImGui::SeparatorText("Preview");

        EnsureFramesGenerated(state);
        if (!state.texture || state.frames.empty() || anim.frames.empty())
        {
            Utils::DrawEmptyStateMessage("No frames to preview.");
            return;
        }

        // Reset au changement d’animation
        if (state.previewAnimationIndex != state.activeAnimation)
        {
            state.previewAnimationIndex = state.activeAnimation;
            state.previewFrame = 0;
            state.previewTimer = 0.0f;
            state.previewPlaying = true;
        }

        const ImGuiIO& io = ImGui::GetIO();
        float delta = io.DeltaTime;
        if (delta <= 0.f) delta = 1.f / 60.f;

        // Frame stepping
        if (state.previewPlaying && anim.frameRate > 0.f)
        {
            state.previewTimer += delta;
            float duration = 1.f / anim.frameRate;
            if (state.previewTimer >= duration)
            {
                state.previewTimer -= duration;
                state.previewFrame++;
                if (state.previewFrame >= static_cast<int>(anim.frames.size()))
                {
                    if (anim.loop)
                        state.previewFrame = 0;
                    else
                    {
                        state.previewFrame = static_cast<int>(anim.frames.size()) - 1;
                        state.previewPlaying = false;
                    }
                }
            }
        }

        size_t frameIdx = anim.frames[std::clamp(state.previewFrame, 0, static_cast<int>(anim.frames.size()) - 1)];
        if (frameIdx >= state.frames.size())
            return;

        const auto& f = state.frames[frameIdx];
        const auto& uv = f.GetUVRect();

        float texW = static_cast<float>(state.texture->GetWidth());
        float texH = static_cast<float>(state.texture->GetHeight());

        const ImVec2 uv0(uv.X / texW, uv.Y / texH);
        const ImVec2 uv1((uv.X + uv.Width) / texW, (uv.Y + uv.Height) / texH);
        const ImVec2 dispSize(uv.Width * state.previewScale, uv.Height * state.previewScale);

        const ImTextureRef frameRef = Utils::ToTextureRef(state.texture->GetNativeHandle());
        if (frameRef.GetTexID() == ImTextureID_Invalid)
        {
            Utils::DrawEmptyStateMessage("Texture preview unavailable.");
            return;
        }

        ImGui::Image(frameRef, dispSize, uv0, uv1);

        // ───────────── Controls ─────────────
        ImGui::Spacing();
        if (ImGui::Button(state.previewPlaying ? "⏸ Pause" : "▶ Play"))
        {
            // Si on reprend après pause, reset le timer pour éviter freeze
            if (!state.previewPlaying)
                state.previewTimer = 0.f;
            state.previewPlaying = !state.previewPlaying;
        }

        ImGui::SameLine();
        if (ImGui::Button("⟲ Restart"))
        {
            state.previewFrame = 0;
            state.previewTimer = 0.f;
            state.previewPlaying = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("◀ Prev"))
        {
            state.previewPlaying = false;
            state.previewFrame = (state.previewFrame - 1 + static_cast<int>(anim.frames.size())) % static_cast<int>(anim
                .frames.size());
        }

        ImGui::SameLine();
        if (ImGui::Button("Next ▶"))
        {
            state.previewPlaying = false;
            state.previewFrame = (state.previewFrame + 1) % static_cast<int>(anim.frames.size());
        }

        ImGui::SameLine();
        ImGui::Text("Frame %d / %zu", state.previewFrame + 1, anim.frames.size());
        ImGui::SliderFloat("Zoom", &state.previewScale, 0.25f, 8.0f, "%.2fx");
    }


    void SpriteAtlasEditorController::DrawSaveSection(SharedState& state)
    {
        if (ImGui::Button("💾 Save Atlas"))
        {
            (void)SaveAtlas(state);
        }

        ImGui::SameLine();
        if (state.dirty)
            ImGui::TextColored(Theme::WarningColor, "Unsaved changes");
        else
            ImGui::TextDisabled("No pending changes.");
    }

    void SpriteAtlasEditorController::RefreshTexture(SharedState& state)
    {
        if (state.texture && state.texture->GetNativeHandle())
            return;

        if (state.textureAbsolutePath.empty())
            return;

        if (!std::filesystem::exists(state.textureAbsolutePath))
        {
            state.error = String("Texture not found: ") + state.textureAbsolutePath.generic_string();
            return;
        }

        auto texture = std::make_shared<resources::Texture>();
        if (!texture->LoadFromFile(String(state.textureAbsolutePath.generic_string())))
        {
            state.error = String("Unable to load texture: ") + state.textureAbsolutePath.generic_string();
            return;
        }

        state.texture = std::move(texture);
        state.frames.clear();
        state.cachedColumns = -1.0f;
        state.cachedRows = -1.0f;
        state.cachedPadding = -1;
        state.cachedMargin = -1;
        state.previewAnimationIndex = -1;
        state.previewFrame = 0;
        state.previewTimer = 0.0f;
    }

    void SpriteAtlasEditorController::EnsureFramesGenerated(SharedState& state)
    {
        if (!state.texture)
        {
            state.frames.clear();
            state.cachedColumns = -1.0f;
            state.cachedRows = -1.0f;
            state.cachedPadding = -1;
            state.cachedMargin = -1;
            return;
        }

        const float columns = std::max(0.0f, state.definition.columns);
        const float rows = std::max(0.0f, state.definition.rows);
        const int expectedCount = static_cast<int>(std::ceil(columns) * std::ceil(rows));
        if (expectedCount <= 0)
        {
            state.frames.clear();
            state.cachedColumns = -1.0f;
            state.cachedRows = -1.0f;
            state.cachedPadding = -1;
            state.cachedMargin = -1;
            return;
        }

        const bool layoutChanged = std::abs(state.cachedColumns - columns) > 0.001f ||
            std::abs(state.cachedRows - rows) > 0.001f ||
            state.cachedPadding != state.definition.padding ||
            state.cachedMargin != state.definition.margin;

        if (!layoutChanged && static_cast<int>(state.frames.size()) == expectedCount)
            return;

        auto frames = resources::SpriteAtlasUtils::GenerateFrames(*state.texture, columns, rows, state.definition.padding, state.definition.margin);
        if (frames.empty())
        {
            state.frames.clear();
            state.cachedColumns = -1.0f;
            state.cachedRows = -1.0f;
            state.cachedPadding = -1;
            state.cachedMargin = -1;
            return;
        }

        state.frames = std::move(frames);
        state.cachedColumns = columns;
        state.cachedRows = rows;
        state.cachedPadding = state.definition.padding;
        state.cachedMargin = state.definition.margin;
    }

    void SpriteAtlasEditorController::EnsureSelectionSize(SharedState& state)
    {
        const int totalFrames = FrameCount(state);
        if (totalFrames <= 0)
        {
            state.frameSelection.clear();
            return;
        }

        if (static_cast<int>(state.frameSelection.size()) != totalFrames)
            state.frameSelection.assign(static_cast<size_t>(totalFrames), false);
    }

    void SpriteAtlasEditorController::ToggleFrameSelection(SharedState& state, int frameIndex, bool appendToSelection)
    {
        if (frameIndex < 0 || frameIndex >= static_cast<int>(state.frameSelection.size()))
            return;

        if (!appendToSelection)
            std::fill(state.frameSelection.begin(), state.frameSelection.end(), false);

        state.frameSelection[frameIndex] = !state.frameSelection[frameIndex];
    }

    void SpriteAtlasEditorController::AssignSelectionToAnimation(SharedState& state, resources::SpriteAnimationDefinition& animation)
    {
        animation.frames.clear();
        for (int index = 0; index < static_cast<int>(state.frameSelection.size()); ++index)
        {
            if (state.frameSelection[index])
                animation.frames.push_back(static_cast<size_t>(index));
        }
    }

    bool SpriteAtlasEditorController::SaveAtlas(SharedState& state)
    {
        if (state.atlasPath.empty())
        {
            state.error = "Atlas path is invalid.";
            return false;
        }

        String saveError;
        if (!resources::SpriteAtlasFactory::SaveAtlasFile(state.atlasPath, state.definition, state.animations, saveError))
        {
            state.error = saveError;
            return false;
        }

        state.error.Clear();
        state.dirty = false;
        LOG_INFO(String{"Sprite atlas saved: "} + state.assetDisplayName);
        return true;
    }

    int SpriteAtlasEditorController::FrameCount(const SharedState& state) const noexcept
    {
        if (state.definition.columns <= 0.001f || state.definition.rows <= 0.001f)
            return 0;
        
        return static_cast<int>(std::ceil(state.definition.columns) * std::ceil(state.definition.rows));
    }

    void SpriteAtlasEditorController::RemoveSelectedAnimation(SharedState& state, int index)
    {
        if (index < 0 || index >= static_cast<int>(state.animations.size()))
            return;

        state.animations.erase(state.animations.begin() + index);
        if (state.animations.empty())
        {
            state.activeAnimation = -1;
            state.previewAnimationIndex = -1;
            state.previewFrame = 0;
            state.previewTimer = 0.0f;
            state.previewPlaying = false;
        }
        else
        {
            state.activeAnimation = std::clamp(index, 0, static_cast<int>(state.animations.size()) - 1);
            state.previewAnimationIndex = -1;
            state.previewFrame = 0;
            state.previewTimer = 0.0f;
        }
        state.dirty = true;
    }
}

#include "Gui/Controllers/Windows/SpriteAtlasEditorWindow.h"
#include <algorithm>
#include <cmath> 
#include <string>
#include "Debug/Logger.h"
#include "Gui/Panels/GuiPanel.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Ressources/Atlas/SpriteAtlasFactory.h"
#include "Ressources/RessourcesClass/Texture.h"
#include "Gui/Core/EditorPreferences.h"
#include "imgui.h"
#include "Gui/Widgets/Metrics/PerformanceColor.h"


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

            return {path.generic_string()};
        }

        float ComputeCellDimension(float totalSize, int count, int padding, int margin)
        {
            if (count <= 0)
                return 0.0f;
            
            const float totalPadding = std::max(0.0f, static_cast<float>(count - 1)) * static_cast<float>(padding);
            const float totalMargins = static_cast<float>(margin * 2);
            
            return (totalSize - totalPadding - totalMargins) / static_cast<float>(count);
        }
    }

    // --- Constructor ---
    SpriteAtlasEditorWindow::SpriteAtlasEditorWindow(std::shared_ptr<BaseAssetEditorWindow::SharedState> sharedState) : BaseAssetEditorWindow(std::move(sharedState),
        {
            .titlePrefix = "Sprite Atlas", 
            .dockRegion = DockSpaceRegion::Center,
            .stableIdSuffix = "Atlas"
        })
    {
        
    }

    SpriteAtlasEditorWindow::~SpriteAtlasEditorWindow() = default;

    // --- Factory ---
    std::shared_ptr<BaseAssetEditorWindow::SharedState> SpriteAtlasEditorWindow::CreateSharedState(const std::filesystem::path& atlasPath,
        String stableIdRoot, std::function<void()> onCloseRequest)
    {
        auto state = std::make_shared<SharedState>();
        state->assetPath = atlasPath;
        state->stableIdRoot = std::move(stableIdRoot);
        state->assetDisplayName = BuildDisplayName(atlasPath);
        state->assetTypeLabel = "Sprite Atlas";
        state->onCloseRequest = std::move(onCloseRequest);

        Resources::SpriteAtlasDefinition definition;
        std::vector<Resources::SpriteAnimationDefinition> animations;

        if (!Resources::SpriteAtlasUtils::ParseAtlasFile(String(atlasPath.generic_string().c_str()), definition, animations))
        {
            state->error = "Failed to parse atlas file.";
        }
        else
        {
            state->definition = definition;
            state->animations = std::move(animations);
            
            const int safeCols = std::max(0, definition.columns);
            const int safeRows = std::max(0, definition.rows);
            
            int totalFrames = static_cast<int>(std::ceil(safeCols) * std::ceil(safeRows));
            state->frameSelection.assign(static_cast<size_t>(totalFrames), false);

            if (!state->animations.empty())
            {
                state->activeAnimation = 0;
                state->previewAnimationIndex = 0;
                state->previewPlaying = true;
            }

            const std::filesystem::path directory = atlasPath.parent_path();
            if (!definition.texturePath.empty())
            {
                state->textureAbsolutePath = directory / definition.texturePath.View();
            }
        }

        return state;
    }

    void SpriteAtlasEditorWindow::DrawCheckerboard(ImDrawList* draw, ImVec2 pos, ImVec2 size, ImU32 col1, ImU32 col2)
    {
        draw->AddRectFilled(pos, {pos.x + size.x, pos.y + size.y}, col1);
        
        const float gridSize = 10.0f;

        const int numCols = static_cast<int>(std::ceil(size.x / gridSize));
        const int numRows = static_cast<int>(std::ceil(size.y / gridSize));

        for (int row = 0; row < numRows; ++row)
        {
            float y = static_cast<float>(row) * gridSize;

            for (int col = 0; col < numCols; ++col)
            {
                if ((col + row) % 2 == 0)
                {
                    float x = static_cast<float>(col) * gridSize;

                    float w = std::min(gridSize, size.x - x);
                    float h = std::min(gridSize, size.y - y);

                    draw->AddRectFilled(
                        {
                            pos.x + x,
                            pos.y + y
                        }, 
                        {
                            pos.x + x + w,
                            pos.y + y + h
                        }, 
                        col2
                    );
                }
            }
        }
    }

    // --- Main Draw Loop ---
    void SpriteAtlasEditorWindow::DrawPanelContents(GuiPanel&)
    {
        auto baseState = GetSharedState();
        auto state = std::static_pointer_cast<SharedState>(baseState);
        
        if (!state)
        {
            GuiUtils::DrawEmptyStateMessage("No sprite atlas selected.");
            return;
        }

        if (!state->error.empty())
        {
            GuiUtils::DrawErrorMessage(std::string(state->error.View()));
        }

        // --- 1. Header Settings ---
        ImGui::TextUnformatted("Atlas Settings");
        ImGui::Separator();
        
        ImGui::Text("File: %s", state->textureAbsolutePath.filename().string().c_str());
        
        bool layoutChanged = false;
        
        if (ImGui::InputInt("Columns", &state->definition.columns))
        {
            layoutChanged = true;
        }
        
        if (ImGui::InputInt("Rows", &state->definition.rows))
        {
            layoutChanged = true;
        }

        int p = state->definition.padding;
        int m = state->definition.margin;
        
        if (ImGui::InputInt("Padding", &p))
        {
            state->definition.padding = std::max(0, p);
            layoutChanged = true;
        }
        
        if (ImGui::InputInt("Margin", &m))
        {
            state->definition.margin = std::max(0, m);
            layoutChanged = true;
        }
        
        if (layoutChanged)
        {
            state->definition.columns = std::max(1, state->definition.columns);
            state->definition.rows = std::max(1, state->definition.rows);
            state->dirty = true;
            
            EnsureFramesGenerated(*state);
            EnsureSelectionSize(*state);
        }

        ImGui::Spacing();

        // --- 2. Atlas Preview ---
        DrawAtlasPreview(*state);

        ImGui::Spacing();
        ImGui::Separator();
        
        // --- 3. Animations Section ---
        ImGui::TextUnformatted("Animations");
        ImGui::Separator();
        DrawAnimationSection(*state);

        ImGui::Spacing();
        ImGui::Separator();
        
        // --- 4. Save Section ---
        DrawSaveSection(*state);
    }

    // --- Section: Atlas Preview (Grid) ---
    void SpriteAtlasEditorWindow::DrawAtlasPreview(SharedState& state)
    {
        EnsureSelectionSize(state);
        RefreshTexture(state);
        EnsureFramesGenerated(state);

        if (!state.texture)
        {
            GuiUtils::DrawEmptyStateMessage("Texture not loaded or path invalid.");
            return;
        }

        const float texW = state.texture->GetWidth();
        const float texH = state.texture->GetHeight();
        
        if (texW <= 0 || texH <= 0) return;

        // Toolbar: Zoom Slider
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Visualisation:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        if (ImGui::SliderFloat("##MainZoom", &state.mainViewScale, 0.1f, 5.0f, "Zoom: %.1fx"))
        {
            // Pas de logique spécifique, juste redraw frame suivante
        }

        const ImVec2 available = ImGui::GetContentRegionAvail();
        
        float dispW = texW * state.mainViewScale;
        float dispH = texH * state.mainViewScale;
        
        float height = std::max(300.0f, available.y * 0.45f);
        
        if (ImGui::BeginChild("AtlasScrollRegion", ImVec2(0, height), true, ImGuiWindowFlags_HorizontalScrollbar))
        {
            const ImVec2 origin = ImGui::GetCursorScreenPos();
            ImDrawList* draw = ImGui::GetWindowDrawList();

            // 1. Background Checkerboard
            DrawCheckerboard(draw, origin, {dispW, dispH}, IM_COL32(50, 50, 50, 255), IM_COL32(80, 80, 80, 255));

            // 2. Texture Image
            const ImTextureRef previewRef = GuiUtils::ToTextureRef(state.texture->GetNativeHandle());
            if (previewRef.GetTexID() != ImTextureID_Invalid)
            {
                ImGui::Image(previewRef, {dispW, dispH});
            }

            // 3. Grid Logic & Interaction
            const float scaleX = dispW / texW;
            const float scaleY = dispH / texH;

            const int cols = std::max(1, state.definition.columns);
            const int rows = std::max(1, state.definition.rows);
            
            const float cellW = ComputeCellDimension(texW, cols, state.definition.padding, state.definition.margin);
            const float cellH = ComputeCellDimension(texH, rows, state.definition.padding, state.definition.margin);

            int hovered = -1;
            int iCols = static_cast<int>(std::ceil(cols));
            int iRows = static_cast<int>(std::ceil(rows));

            // Couleurs
            const auto& settings = EditorSettings::Get();
            const ImU32 accentColor = ImGui::ColorConvertFloat4ToU32(settings.ThemeAccentColor);
            const ImU32 selectionFill = (accentColor & 0x00FFFFFF) | 0x55000000;
            const ImU32 hoverFill = IM_COL32(255, 255, 255, 40);
            const ImU32 gridColor = IM_COL32(200, 200, 200, 100);

            const float fMargin = static_cast<float>(state.definition.margin);
            const float fPadding = static_cast<float>(state.definition.padding);

            // Draw Grid Cells
            for (int y = 0; y < iRows; ++y)
            {
                for (int x = 0; x < iCols; ++x)
                {
                    const float xF = static_cast<float>(x);
                    const float yF = static_cast<float>(y);

                    const float x0 = fMargin + xF * (cellW + fPadding);
                    const float y0 = fMargin + yF * (cellH + fPadding);
        
                    const float x1 = x0 + cellW;
                    const float y1 = y0 + cellH;

                    ImVec2 min(origin.x + x0 * scaleX, origin.y + y0 * scaleY);
                    ImVec2 max(origin.x + x1 * scaleX, origin.y + y1 * scaleY);

                    const int idx = y * iCols + x;
                    
                    // Mouse Interaction
                    if (ImGui::IsMouseHoveringRect(min, max))
                    {
                        hovered = idx;
                    }

                    // Render
                    bool isSelected = (idx < static_cast<int>(state.frameSelection.size())) && state.frameSelection[idx];

                    if (isSelected)
                    {
                        draw->AddRectFilled(min, max, selectionFill);
                    }
                    else if (hovered == idx)
                    {
                        draw->AddRectFilled(min, max, hoverFill);
                    }
                    
                    draw->AddRect(min, max, gridColor);
                }
            }

            // Click
            if (hovered >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
            {
                HandleSelectionInput(state, hovered);
            }

            // Tooltip
            if (hovered >= 0)
            {
                ImGui::BeginTooltip();
                ImGui::Text("Frame: %d\nCoords: (%d, %d)", hovered, hovered % iCols, hovered / iCols);
                ImGui::EndTooltip();
            }
        }
        ImGui::EndChild();
    }

    // --- Input Logic (Ctrl/Shift) ---
    void SpriteAtlasEditorWindow::HandleSelectionInput(SharedState& state, int hoveredIndex)
    {
        const ImGuiIO& io = ImGui::GetIO();
        
        // 1. Shift + Click
        if (io.KeyShift && state.lastSelectedFrameIndex != -1)
        {
            int start = std::min(state.lastSelectedFrameIndex, hoveredIndex);
            int end = std::max(state.lastSelectedFrameIndex, hoveredIndex);
            
            if (!io.KeyCtrl) 
            {
                state.frameSelection.assign(state.frameSelection.size(), false);
            }

            for (int i = start; i <= end; ++i)
            {
                if (i < static_cast<int>(state.frameSelection.size())) 
                    state.frameSelection[i] = true;
            }
        }
        // 2. Ctrl + Click
        else if (io.KeyCtrl)
        {
             if (hoveredIndex >= 0 && static_cast<size_t>(hoveredIndex) < state.frameSelection.size()) 
             {
                 state.frameSelection[hoveredIndex] = !state.frameSelection[hoveredIndex];
                 
                 if (state.frameSelection[hoveredIndex]) 
                     state.lastSelectedFrameIndex = hoveredIndex;
             }
        }
        // 3. Simple Click
        else
        {
            state.frameSelection.assign(state.frameSelection.size(), false);
            
            if (hoveredIndex >= 0 && static_cast<size_t>(hoveredIndex) < state.frameSelection.size()) 
            {
                state.frameSelection[hoveredIndex] = true;
                state.lastSelectedFrameIndex = hoveredIndex;
            }
        }
    }

    // --- Section: Animations List & Settings ---
    void SpriteAtlasEditorWindow::DrawAnimationSection(SharedState& state)
    {
        if (ImGui::Button("New Animation"))
        {
            Resources::SpriteAnimationDefinition anim;
            anim.name = String("Anim_") + String(std::to_string(state.animations.size() + 1));
            anim.frameRate = 12.0f;
            anim.loop = true;
            state.animations.push_back(anim);
            
            state.activeAnimation = static_cast<int>(state.animations.size()) - 1;
            state.previewAnimationIndex = -1;
            state.dirty = true;
        }

        ImGui::SameLine();
        bool hasSelection = (state.activeAnimation >= 0 && static_cast<size_t>(state.activeAnimation) < state.animations.size());
        
        ImGui::BeginDisabled(!hasSelection);
        if (ImGui::Button("Delete"))
        {
            RemoveSelectedAnimation(state, state.activeAnimation);
        }
        
        ImGui::EndDisabled();

        ImGui::Separator();

        if (ImGui::BeginTable("AnimTable", 2, ImGuiTableFlags_BordersInner | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("List", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();

            // --- Column 1: List ---
            ImGui::TableSetColumnIndex(0);
            if (ImGui::BeginChild("AnimListC", ImVec2(0, 350), true))
            {
                for (int i = 0; i < static_cast<int>(state.animations.size()); ++i)
                {
                    bool isSelected = (state.activeAnimation == i);
                    std::string label = state.animations[i].name.empty() ? "Anim " + std::to_string(i) : state.animations[i].name.Std();

                    if (ImGui::Selectable(label.c_str(), isSelected))
                    {
                        state.activeAnimation = i;
                        state.previewAnimationIndex = -1;
                        state.previewFrame = 0;
                        state.previewTimer = 0.0f;
                    }
                }
            }
            ImGui::EndChild();

            // --- Column 2: Details ---
            ImGui::TableSetColumnIndex(1);
            if (hasSelection)
            {
                auto& anim = state.animations[state.activeAnimation];

                char buffer[128] = {};
                std::string currentName = anim.name.Std();
                std::ranges::copy(currentName, buffer);
                
                if (ImGui::InputText("Name", buffer, sizeof(buffer)))
                {
                    anim.name = buffer;
                    state.dirty = true;
                }

                // Settings
                if (ImGui::DragFloat("Frame Rate", &anim.frameRate, 0.1f, 0.1f, 120.0f, "%.2f")) 
                    state.dirty = true;
                
                if (ImGui::Checkbox("Loop", &anim.loop)) 
                    state.dirty = true;

                ImGui::Separator();
                
                ImGui::Text("Frames: %zu assigned", anim.frames.size());
                
                if (ImGui::Button("Assign Selected Frames from Atlas"))
                {
                    AssignSelectionToAnimation(state, anim);
                    state.dirty = true;
                }
                
                ImGui::SameLine();
                if (ImGui::Button("Clear Frames"))
                {
                    anim.frames.clear();
                    state.dirty = true;
                }

                // Preview Box
                DrawAnimationPreview(state, anim);
            }
            else
            {
                GuiUtils::DrawEmptyStateMessage("Select or create an animation to edit.");
            }

            ImGui::EndTable();
        }
    }

    // --- Section: Animation Playback Preview ---
    void SpriteAtlasEditorWindow::DrawAnimationPreview(SharedState& state, Resources::SpriteAnimationDefinition& anim)
    {
        ImGui::Spacing();
        ImGui::SeparatorText("Preview");

        if (!state.texture || anim.frames.empty())
        {
            ImGui::TextDisabled("No frames or texture available.");
            return;
        }

        // --- Logic: Update Animation Timer ---
        if (state.previewPlaying && anim.frameRate > 0.0f)
        {
            float dt = ImGui::GetIO().DeltaTime;
            state.previewTimer += dt;
            
            float duration = 1.0f / anim.frameRate;
            if (state.previewTimer >= duration)
            {
                state.previewTimer -= duration;
                state.previewFrame++;
            }
        }

        // Clamp / Loop Frame Index
        int frameCount = static_cast<int>(anim.frames.size());
        if (state.previewFrame >= frameCount)
        {
            if (anim.loop && frameCount > 0)
            {
                state.previewFrame = 0;
            }
            else
            {
                state.previewFrame = std::max(0, frameCount - 1);
                state.previewPlaying = false;
            }
        }

        size_t globalFrameIndex = anim.frames[state.previewFrame];
        if (globalFrameIndex >= state.frames.size())
            return;

        const auto& f = state.frames[globalFrameIndex];
        const auto uv = f.GetUVRect();

        // --- Render ---
        float texW = state.texture->GetWidth();
        float texH = state.texture->GetHeight();

        ImVec2 uv0(uv.x / texW, uv.y / texH);
        ImVec2 uv1((uv.x + uv.width) / texW, (uv.y + uv.height) / texH);
        
        ImVec2 size(uv.width * state.previewScale, uv.height * state.previewScale);

        float availWidth = ImGui::GetContentRegionAvail().x;
        float offsetX = (availWidth - size.x) * 0.5f;
        if (offsetX > 0)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* draw = ImGui::GetWindowDrawList();

        DrawCheckerboard(draw, pos, size, IM_COL32(60,60,60,255), IM_COL32(90,90,90,255));

        ImGui::Image(GuiUtils::ToTextureRef(state.texture->GetNativeHandle()), size, uv0, uv1);

        ImGui::Spacing();
        ImGui::Separator();

        float controlsW = 220.0f;
        if (availWidth > controlsW) 
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - controlsW) * 0.5f);

        if (ImGui::Button("⏮"))
            state.previewFrame = 0;
        
        ImGui::SameLine();
        
        if (ImGui::Button(state.previewPlaying ? " ⏸ " : " ▶ ")) 
            state.previewPlaying = !state.previewPlaying;
        
        ImGui::SameLine();
        if (ImGui::Button("⏭")) 
            state.previewFrame = (state.previewFrame + 1) % frameCount;

        ImGui::SameLine();
        ImGui::Text("%d / %d", state.previewFrame + 1, frameCount);

        ImGui::SliderFloat("Scale", &state.previewScale, 0.5f, 10.0f, "%.1fx");
    }

    // --- Section: Save Button ---
    void SpriteAtlasEditorWindow::DrawSaveSection(SharedState& state)
    {
        if (ImGui::Button("Save Atlas"))
        {
            SaveAtlas(state);
        }

        ImGui::SameLine();
        
        if (state.dirty)
        {
            ImGui::TextColored(Widgets::Metrics::WarningColor(), "Unsaved changes");
        }
        else
        {
            ImGui::TextDisabled("No pending changes.");
        }
    }

    // --- Helpers Implementation ---

    void SpriteAtlasEditorWindow::RefreshTexture(SharedState& state)
    {
        if (state.texture && state.texture->GetNativeHandle())
            return;

        if (state.textureAbsolutePath.empty())
            return;

        if (!std::filesystem::exists(state.textureAbsolutePath))
        {
            state.error = String("Texture not found: " + state.textureAbsolutePath.generic_string());
            return;
        }

        auto texture = std::make_shared<Resources::Texture>();
        if (!texture->LoadFromFile(String(state.textureAbsolutePath.generic_string())))
        {
            state.error = String("Unable to load texture: " + state.textureAbsolutePath.generic_string());
            return;
        }

        state.texture = std::move(texture);
        
        state.cachedColumns = -1; 
        state.frames.clear();
    }

    void SpriteAtlasEditorWindow::EnsureFramesGenerated(SharedState& state)
    {
        if (!state.texture)
            return;

        int cols = std::max(1, state.definition.columns);
        int rows = std::max(1, state.definition.rows);
        
        int pad = state.definition.padding;
        int margin = state.definition.margin;

        // Check cache
        bool changed = (state.cachedColumns != cols) ||
                       (state.cachedRows != rows) ||
                       (state.cachedPadding != pad) ||
                       (state.cachedMargin != margin) ||
                       state.frames.empty();

        if (changed)
        {
            auto newFrames = Resources::SpriteAtlasUtils::GenerateFrames(*state.texture, cols, rows, pad, margin);
            state.frames = std::move(newFrames);
            
            state.cachedColumns = cols;
            state.cachedRows = rows;
            state.cachedPadding = pad;
            state.cachedMargin = margin;
        }
    }

    void SpriteAtlasEditorWindow::EnsureSelectionSize(SharedState& state)
    {
        int count = FrameCount(state);
        if (count != static_cast<int>(state.frameSelection.size()))
        {
            state.frameSelection.resize(count, false);
        }
    }

    void SpriteAtlasEditorWindow::AssignSelectionToAnimation(SharedState& state, Resources::SpriteAnimationDefinition& animation)
    {
        animation.frames.clear();
        for (size_t i = 0; i < state.frameSelection.size(); ++i)
        {
            if (state.frameSelection[i])
            {
                animation.frames.push_back(i);
            }
        }
    }

    void SpriteAtlasEditorWindow::RemoveSelectedAnimation(SharedState& state, int index)
    {
        if (index < 0 || index >= static_cast<int>(state.animations.size()))
            return;

        state.animations.erase(state.animations.begin() + index);
        
        if (state.animations.empty())
        {
            state.activeAnimation = -1;
        }
        else
        {
            state.activeAnimation = std::clamp(index, 0, static_cast<int>(state.animations.size()) - 1);
        }
        
        state.dirty = true;
    }

    bool SpriteAtlasEditorWindow::SaveAtlas(SharedState& state)
    {
        if (state.assetPath.empty())
        {
            state.error = "Invalid atlas path.";
            return false;
        }

        String err;
        if (Resources::SpriteAtlasFactory::SaveAtlasFile(state.assetPath, state.definition, state.animations, err))
        {
            state.dirty = false;
            state.error.clear();
            LOG_INFO("Sprite Atlas Saved: " + state.assetDisplayName);
            return true;
        }
        else
        {
            state.error = err;
            LOG_ERROR("Failed to save atlas: " + std::string(err.View()));
            return false;
        }
    }

    int SpriteAtlasEditorWindow::FrameCount(const SharedState& state) const noexcept
    {
        int c = std::max(0, state.definition.columns);
        int r = std::max(0, state.definition.rows);
        
        return static_cast<int>(std::ceil(c) * std::ceil(r));
    }

    void SpriteAtlasEditorWindow::OnSaveRequested()
    {
        if (auto state = std::static_pointer_cast<SharedState>(GetSharedState()))
        {
            SaveAtlas(*state);
        }
    }
}

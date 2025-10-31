#include "Engine/Gui/Controllers/SpriteAtlasEditorController.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <format>

#include "Core/Logger.h"
#include "Engine/Gui/Internal/GuiPanel.h"
#include "Engine/Gui/Utils/GuiHelpers.h"
#include "Engine/Ressources/SpriteAtlasFactory.h"
#include "Engine/Ressources/Texture.h"
#include "imgui.h"

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

        float ComputeCellDimension(float totalSize, int count, int padding, int margin)
        {
            if (count <= 0)
                return 0.0f;
            const float totalPadding = static_cast<float>(std::max(0, count - 1) * padding);
            const float totalMargins = static_cast<float>(margin * 2);
            return (totalSize - totalPadding - totalMargins) / static_cast<float>(count);
        }
    }

    SpriteAtlasEditorController::SpriteAtlasEditorController(std::shared_ptr<SharedState> sharedState)
        : BaseAssetEditorController(std::move(sharedState), {.titlePrefix = "Sprite Atlas", .dockRegion = DockSpaceRegion::Center, .stableIdSuffix = "Atlas"})
    {
    }

    std::shared_ptr<SpriteAtlasEditorController::SharedState> SpriteAtlasEditorController::CreateSharedState(const std::filesystem::path& atlasPath, String stableIdRoot, std::function<void()> onCloseRequest)
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
        if (!resources::SpriteAtlasUtils::ParseAtlasFile(String(atlasPath.generic_string().c_str()), definition, animations))
        {
            state->error = "Failed to parse atlas file.";
        }
        else
        {
            state->definition = definition;
            state->animations = std::move(animations);
            const int safeColumns = std::max(0, definition.columns);
            const int safeRows = std::max(0, definition.rows);
            state->frameSelection.assign(static_cast<size_t>(safeColumns * safeRows), false);
            if (!state->animations.empty())
                state->activeAnimation = 0;

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

        if (!state->error.IsEmpty())
            Utils::DrawErrorMessage(std::string(state->error.View()));

        DrawAtlasPreview(*state);
        ImGui::Spacing();
        DrawAnimationSection(*state);
        ImGui::Spacing();
        DrawSaveSection(*state);
    }

    void SpriteAtlasEditorController::OnSaveRequested()
    {
        auto baseState = GetSharedState();
        auto state = std::static_pointer_cast<SharedState>(baseState);
        if (!state)
            return;

        SaveAtlas(*state);
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

        const float textureWidth = static_cast<float>(state.texture->GetWidth());
        const float textureHeight = static_cast<float>(state.texture->GetHeight());
        if (textureWidth <= 0.0f || textureHeight <= 0.0f)
        {
            Utils::DrawEmptyStateMessage("Texture has invalid dimensions.");
            return;
        }

        const ImVec2 available = ImGui::GetContentRegionAvail();
        const float aspect = textureHeight / textureWidth;
        float displayWidth = available.x;
        float displayHeight = displayWidth * aspect;
        if (displayHeight > available.y * 0.6f && available.y > 0.0f)
        {
            displayHeight = available.y * 0.6f;
            displayWidth = displayHeight / aspect;
        }

        const ImVec2 imageSize(displayWidth, displayHeight);
        const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        ImGui::Image(state.texture->GetNativeHandle(), imageSize);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const float scaleX = displayWidth / textureWidth;
        const float scaleY = displayHeight / textureHeight;
        const int columns = std::max(1, state.definition.columns);
        const int rows = std::max(1, state.definition.rows);
        const float cellWidth = ComputeCellDimension(textureWidth, columns, state.definition.padding, state.definition.margin);
        const float cellHeight = ComputeCellDimension(textureHeight, rows, state.definition.padding, state.definition.margin);

        if (cellWidth <= 0.0f || cellHeight <= 0.0f)
        {
            Utils::DrawEmptyStateMessage("Atlas definition produces invalid cells.");
            return;
        }

        int hoveredFrame = -1;
        const bool imageHovered = ImGui::IsItemHovered();

        for (int row = 0; row < rows; ++row)
        {
            for (int column = 0; column < columns; ++column)
            {
                const float x0 = static_cast<float>(state.definition.margin) + static_cast<float>(column) * (cellWidth + static_cast<float>(state.definition.padding));
                const float y0 = static_cast<float>(state.definition.margin) + static_cast<float>(row) * (cellHeight + static_cast<float>(state.definition.padding));
                const float x1 = x0 + cellWidth;
                const float y1 = y0 + cellHeight;

                const ImVec2 min(cursorPos.x + x0 * scaleX, cursorPos.y + y0 * scaleY);
                const ImVec2 max(cursorPos.x + x1 * scaleX, cursorPos.y + y1 * scaleY);

                const int index = row * columns + column;
                if (index < static_cast<int>(state.frameSelection.size()) && state.frameSelection[index])
                {
                    drawList->AddRectFilled(min, max, IM_COL32(96, 180, 255, 60));
                }

                drawList->AddRect(min, max, IM_COL32(255, 255, 255, 90));

                if (imageHovered && ImGui::IsMouseHoveringRect(min, max))
                {
                    hoveredFrame = index;
                }
            }
        }

        if (hoveredFrame >= 0)
        {
            const bool append = ImGui::GetIO().KeyShift;
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                ToggleFrameSelection(state, hoveredFrame, append);
            }
        }
    }

    void SpriteAtlasEditorController::DrawAnimationSection(SharedState& state)
    {
        ImGui::TextUnformatted("Animations");
        ImGui::Separator();

        if (ImGui::Button("➕ Add"))
        {
            resources::SpriteAnimationDefinition animation;
            animation.name = String("Animation ") + String(std::to_string(state.animations.size() + 1));
            animation.frameRate = 8.0f;
            animation.loop = true;
            state.animations.push_back(std::move(animation));
            state.activeAnimation = static_cast<int>(state.animations.size()) - 1;
            state.dirty = true;
        }

        ImGui::SameLine();
        const bool canModify = state.activeAnimation >= 0 && state.activeAnimation < static_cast<int>(state.animations.size());
        ImGui::BeginDisabled(!canModify);
        if (ImGui::Button("✏️ Rename"))
        {
            auto& animation = state.animations[state.activeAnimation];
            const std::string currentName = animation.name.IsEmpty() ? std::string{} : std::string(animation.name.View());
            std::snprintf(state.renameBuffer, IM_ARRAYSIZE(state.renameBuffer), "%s", currentName.c_str());
            ImGui::OpenPopup("SpriteAtlasRenameAnimation");
        }

        ImGui::SameLine();
        if (ImGui::Button("❌ Delete"))
        {
            RemoveSelectedAnimation(state, state.activeAnimation);
        }
        ImGui::EndDisabled();

        if (ImGui::BeginPopupModal("SpriteAtlasRenameAnimation", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (ImGui::IsWindowAppearing())
                ImGui::SetKeyboardFocusHere();

            ImGui::InputText("New name", state.renameBuffer, IM_ARRAYSIZE(state.renameBuffer));

            Utils::DrawConfirmButtons("Rename", "Cancel",
                [&]()
                {
                    if (state.activeAnimation >= 0 && state.activeAnimation < static_cast<int>(state.animations.size()))
                    {
                        auto& animation = state.animations[state.activeAnimation];
                        animation.name = state.renameBuffer;
                        state.dirty = true;
                    }

                    ImGui::CloseCurrentPopup();
                },
                []()
                {
                    ImGui::CloseCurrentPopup();
                });

            ImGui::EndPopup();
        }

        if (ImGui::BeginTable("AtlasAnimationTable", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Animations", ImGuiTableColumnFlags_WidthStretch, 0.35f);
            ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch, 0.65f);

            ImGui::TableNextColumn();
            if (ImGui::BeginChild("AtlasAnimationList", ImVec2(0, 200), true))
            {
                if (state.animations.empty())
                {
                    Utils::DrawEmptyStateMessage("No animations defined.");
                }
                else
                {
                    for (int i = 0; i < static_cast<int>(state.animations.size()); ++i)
                    {
                        const bool selected = (i == state.activeAnimation);
                        const String& name = state.animations[i].name;
                        const std::string label = name.IsEmpty() ? ("Animation " + std::to_string(i + 1)) : std::string(name.View());
                        if (ImGui::Selectable(label.c_str(), selected))
                            state.activeAnimation = i;
                    }
                }
            }
            ImGui::EndChild();

            ImGui::TableNextColumn();
            if (!canModify)
            {
                Utils::DrawEmptyStateMessage("Select an animation to edit.");
            }
            else
            {
                auto& animation = state.animations[state.activeAnimation];
                char nameBuffer[128]{};
                std::string currentName = animation.name.IsEmpty() ? std::string{} : std::string(animation.name.View());
                std::strncpy(nameBuffer, currentName.c_str(), sizeof(nameBuffer) - 1);
                if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
                {
                    animation.name = nameBuffer;
                    state.dirty = true;
                }

                if (ImGui::DragFloat("Frame Rate", &animation.frameRate, 0.1f, 0.0f, 120.0f, "%.2f"))
                {
                    animation.frameRate = std::max(0.0f, animation.frameRate);
                    state.dirty = true;
                }

                if (ImGui::Checkbox("Loop", &animation.loop))
                    state.dirty = true;

                if (ImGui::Button("Use selected frames"))
                {
                    AssignSelectionToAnimation(state, animation);
                    state.dirty = true;
                }

                ImGui::SameLine();
                if (ImGui::Button("Clear frames"))
                {
                    animation.frames.clear();
                    state.dirty = true;
                }

                std::string framesList;
                framesList.reserve(animation.frames.size() * 4);
                for (size_t idx = 0; idx < animation.frames.size(); ++idx)
                {
                    framesList += std::to_string(animation.frames[idx]);
                    if (idx + 1 < animation.frames.size())
                        framesList += ", ";
                }

                ImGui::TextWrapped("Frames: %s", framesList.empty() ? "<none>" : framesList.c_str());
            }

            ImGui::EndTable();
        }
    }

    void SpriteAtlasEditorController::DrawSaveSection(SharedState& state)
    {
        if (ImGui::Button("💾 Save Atlas"))
        {
            SaveAtlas(state);
        }

        ImGui::SameLine();
        if (state.dirty)
            ImGui::TextColored(ImVec4(0.9f, 0.75f, 0.2f, 1.0f), "Unsaved changes");
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
        if (state.definition.columns <= 0 || state.definition.rows <= 0)
            return 0;
        return state.definition.columns * state.definition.rows;
    }

    void SpriteAtlasEditorController::RemoveSelectedAnimation(SharedState& state, int index)
    {
        if (index < 0 || index >= static_cast<int>(state.animations.size()))
            return;

        state.animations.erase(state.animations.begin() + index);
        if (state.animations.empty())
            state.activeAnimation = -1;
        else
            state.activeAnimation = std::clamp(index, 0, static_cast<int>(state.animations.size()) - 1);
        state.dirty = true;
    }
}

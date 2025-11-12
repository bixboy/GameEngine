#include "Gui/Controllers/ComponentEditorController.h"
#include <utility>
#include "Logger.h"
#include "Gui/Utils/GuiHelpers.h"
#include "imgui.h"
#include "Gui/Controllers/BaseAssetEditorController.h"

namespace BixEngine::Gui
{
    namespace
    {
        String BuildDisplayName(const std::filesystem::path& path)
        {
            if (path.empty())
                return "Component Prefab";

            const auto filename = path.filename().generic_string();
            if (!filename.empty())
                return filename;

            const auto stem = path.stem().generic_string();
            if (!stem.empty())
                return stem;

            return String(path.generic_string());
        }

        BaseAssetEditorController::PanelConfig MakePanelConfig(ComponentEditorController::Section section)
        {
            using Section = ComponentEditorController::Section;
            BaseAssetEditorController::PanelConfig config{};

            switch (section)
            {
            case Section::Toolbar:
                config.titlePrefix = "Component Toolbar";
                config.dockRegion = DockSpaceRegion::Top;
                config.stableIdSuffix = "Toolbar";
                break;
            case Section::Inspector:
                config.titlePrefix = "Component Inspector";
                config.dockRegion = DockSpaceRegion::Right;
                config.stableIdSuffix = "Inspector";
                break;
            }

            return config;
        }
    }

    ComponentEditorController::ComponentEditorController(std::shared_ptr<SharedState> sharedState, Section section)
        : BaseAssetEditorController(std::move(sharedState), MakePanelConfig(section)), section_(section)
    {
    }

    std::shared_ptr<ComponentEditorController::SharedState> ComponentEditorController::CreateSharedState(
        std::filesystem::path assetPath, String stableIdRoot, std::function<void()> onCloseRequest)
    {
        auto state = std::make_shared<SharedState>();
        state->assetPath = std::move(assetPath);
        state->stableIdRoot = std::move(stableIdRoot);
        state->onCloseRequest = std::move(onCloseRequest);
        state->assetDisplayName = BuildDisplayName(state->assetPath);
        state->assetTypeLabel = "Component Prefab";
        return state;
    }

    void ComponentEditorController::DrawPanelContents(GuiPanel& panel)
    {
        static_cast<void>(panel);

        switch (section_)
        {
        case Section::Toolbar:
            DrawToolbar();
            break;
        case Section::Inspector:
            DrawInspector();
            break;
        }
    }

    void ComponentEditorController::OnPlayRequested()
    {
        if (const auto state = GetSharedState())
            LOG_INFO(String{"[ComponentPrefabEditor] ▶ Play requested for asset: "} + state->assetDisplayName);
    }

    void ComponentEditorController::OnSaveRequested()
    {
        if (const auto state = GetSharedState())
            LOG_INFO(String{"[ComponentPrefabEditor] 💾 Save requested for asset: "} + state->assetDisplayName);
    }

    void ComponentEditorController::OnCompileRequested()
    {
        if (const auto state = GetSharedState())
            LOG_INFO(String{"[ComponentPrefabEditor] 🧠 Compile requested for asset: "} + state->assetDisplayName);
    }

    void ComponentEditorController::DrawToolbar()
    {
        DrawStandardToolbar();
    }

    void ComponentEditorController::DrawInspector()
    {
        const auto state = GetSharedState();
        if (!state)
        {
            Utils::DrawEmptyStateMessage("No asset selected.");
            return;
        }

        Utils::DrawDescriptionText("Component prefab metadata");
        ImGui::Spacing();

        Utils::DrawLabelValue("Name", state->assetDisplayName.View().data(), "Prefab");
        Utils::DrawLabelValue("Path", state->assetPath.generic_string(), "");
        Utils::DrawLabelValue("Script", state->primaryClassName.empty() ? "Unknown" : state->primaryClassName.c_str(),
                              "Unknown");
        if (!state->includePath.empty())
            Utils::DrawLabelValue("Include", state->includePath.c_str(), "");
    }
}

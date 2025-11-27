#include "Gui/Controllers/ComponentEditorController.h"
#include <fstream>
#include <string_view>
#include <utility>
#include <nlohmann/json.hpp>
#include "Debug/Logger.h"
#include "Gui/Utils/GuiHelpers.h"
#include "imgui.h"
#include "Gui/Controllers/BaseAssetEditorController.h"
#include "Gui/Panels/ActorInspector/PropertyInspector.h"

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

        bool SavePrefabVariables(const BaseAssetEditorController::SharedState& state, std::string_view logPrefix)
        {
            if (state.assetPath.empty())
                return false;

            std::ifstream input(state.assetPath);
            if (!input.is_open())
            {
                LOG_WARNING(String{logPrefix} + " Failed to open prefab for saving: " + state.assetDisplayName);
                return false;
            }

            nlohmann::json document = nlohmann::json::parse(input, nullptr, false);
            if (document.is_discarded() || !document.is_object())
            {
                LOG_WARNING(String{logPrefix} + " Invalid prefab format: " + state.assetDisplayName);
                return false;
            }

            nlohmann::json variables = nlohmann::json::array();
            for (const auto& variable : state.exposedVariables)
            {
                nlohmann::json entry = nlohmann::json::object();

                if (!variable.name.IsEmpty())
                    entry["name"] = variable.name.Std();

                if (!variable.type.IsEmpty())
                    entry["type"] = variable.type.Std();

                if (!variable.value.IsEmpty())
                    entry["default"] = variable.value.Std();

                variables.push_back(std::move(entry));
            }

            document["variables"] = std::move(variables);

            std::ofstream output(state.assetPath);
            if (!output.is_open())
            {
                LOG_WARNING(String{logPrefix} + " Failed to write prefab: " + state.assetDisplayName);
                return false;
            }

            output << document.dump(4);

            LOG_INFO(String{logPrefix} + " 💾 Saved variables for asset: " + state.assetDisplayName);
            return true;
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
            SavePrefabVariables(*state, "[ComponentPrefabEditor]");
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
        Utils::DrawLabelValue("Script", state->primaryClassName.empty() ? "Unknown" : state->primaryClassName.c_str(), "Unknown");

        if (!state->includePath.empty())
            Utils::DrawLabelValue("Include", state->includePath.c_str(), "");

        ActorInspector::PropertyInspector::DrawExposedVariablesSection(*state, "Variables du composant", "Aucune variable exposée pour ce composant.");
    }
}

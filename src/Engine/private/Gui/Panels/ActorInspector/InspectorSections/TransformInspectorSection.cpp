#include "Gui/Panels/ActorInspector/InspectorSections/TransformInspectorSection.h"

#include "Gui/Widgets/Widgets.h"
#include "Gui/Panels/ActorInspector/Utils/ActorInspectorHelpers.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Framework/Actor.h"
#include "Gui/Core/EditorPreferences.h"
#include "Math/Rotator.h"
#include "Math/Vector3.h"
#include <algorithm>
#include <string>


namespace BixEngine::Gui::ActorInspector
{
    using namespace Theme;
    using namespace Utils;
    using namespace BixEngine::Gui::Widgets;

    void TransformInspectorSection::Draw(Game::Actor& actor)
    {
        const std::string contextId = BuildActorContextId(actor);
        PersistentSectionScope section("Transform", contextId);
        if (!section.IsOpen())
        {
            return;
        }

        SectionContainer container("TransformSection");
        if (!container.IsVisible())
        {
            return;
        }

        DrawSeparatorText("Transform");

        ImGui::TextUnformatted("Transform Controls");
        ImGui::SameLine();
        DrawHelpMarker("Reset axes with the coloured buttons or drag values to fine tune the transform.");

        const auto& settings = EditorSettings::Get();

        Math::Vector3 position = actor.GetPosition();
        float positionValues[3] = {position.x, position.y, position.z};
        if (DrawVector3Control("Location", positionValues, 0.0f, settings.DragSpeedLocation, "%.3f"))
        {
            position.x = positionValues[0];
            position.y = positionValues[1];
            position.z = positionValues[2];
            actor.SetPosition(position);
        }

        Math::Rotator rotation = actor.GetRotation();
        float rotationValues[3] = {rotation.pitch, rotation.yaw, rotation.roll};
        if (DrawVector3Control("Rotation", rotationValues, 0.0f, settings.DragSpeedRotation, "%.2f"))
        {
            rotation.pitch = std::clamp(rotationValues[0], -360.0f, 360.0f);
            rotation.yaw = std::clamp(rotationValues[1], -360.0f, 360.0f);
            rotation.roll = std::clamp(rotationValues[2], -360.0f, 360.0f);
            actor.SetRotation(rotation);
        }

        Math::Vector3 scale = actor.GetScale();
        float scaleValues[3] = {scale.x, scale.y, scale.z};
        if (DrawVector3Control("Scale", scaleValues, 1.0f, settings.DragSpeedScale, "%.3f"))
        {
            scale.x = scaleValues[0];
            scale.y = scaleValues[1];
            scale.z = scaleValues[2];
            actor.SetScale(scale);
        }
    }
}

#include "Bix/Engine/Gui/Panels/ActorInspector/TransformSectionUI.h"

#include "Bix/Engine/Gui/Panels/ActorInspector/ImGuiControls.h"
#include "Bix/Engine/Gui/Panels/ActorInspector/Utils/ActorInspectorHelpers.h"

#include "Bix/Engine/Gui/Utils/GuiHelpers.h"
#include "Bix/Game/Actor.h"
#include "Bix/Math/Rotator.h"
#include "Bix/Math/Vector3.h"

#include <algorithm>
#include <string>

namespace BixEngine::Gui::ActorInspector
{
    void DrawTransformSection(Game::Actor& actor)
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

        Utils::DrawSeparatorText("Transform");

        ImGui::TextUnformatted("Transform Controls");
        ImGui::SameLine();
        Utils::DrawHelpMarker("Reset axes with the coloured buttons or drag values to fine tune the transform.");

        Math::Vector3 position = actor.GetPosition();
        float positionValues[3] = {position.x, position.y, position.z};
        if (DrawVector3Control("Location", positionValues, 0.0f, 0.1f, "%.3f"))
        {
            position.x = positionValues[0];
            position.y = positionValues[1];
            position.z = positionValues[2];
            actor.SetPosition(position);
        }

        Math::Rotator rotation = actor.GetRotation();
        float rotationValues[3] = {rotation.pitch, rotation.yaw, rotation.roll};
        if (DrawVector3Control("Rotation", rotationValues, 0.0f, 0.1f, "%.2f"))
        {
            rotation.pitch = std::clamp(rotationValues[0], -360.0f, 360.0f);
            rotation.yaw = std::clamp(rotationValues[1], -360.0f, 360.0f);
            rotation.roll = std::clamp(rotationValues[2], -360.0f, 360.0f);
            actor.SetRotation(rotation);
        }

        Math::Vector3 scale = actor.GetScale();
        float scaleValues[3] = {scale.x, scale.y, scale.z};
        if (DrawVector3Control("Scale", scaleValues, 1.0f, 0.05f, "%.3f"))
        {
            scale.x = scaleValues[0];
            scale.y = scaleValues[1];
            scale.z = scaleValues[2];
            actor.SetScale(scale);
        }
    }
}


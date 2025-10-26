#include "Engine/Gui/Panels/ActorInspector/ReflectionDrawer.h"

#include "Engine/Gui/Panels/ActorInspector/ImGuiControls.h"
#include "Engine/Gui/Utils/GuiHelpers.h"

#include "Core/Math/Rotator.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Containers/String.h"

#include "SDL3/SDL.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cfloat>
#include <cstdint>
#include <cstring>
#include <imgui.h>
#include <string>

namespace BixEngine::Gui::ActorInspector
{
    namespace PropertyUtils
    {
        bool MatchesType(const std::string& typeName, std::string_view expectedSuffix)
        {
            if (typeName.size() < expectedSuffix.size())
            {
                return false;
            }

            const std::size_t offset = typeName.size() - expectedSuffix.size();
            if (typeName.compare(offset, expectedSuffix.size(), expectedSuffix) != 0)
            {
                return false;
            }

            if (offset == 0)
            {
                return true;
            }

            const char preceding = typeName[offset - 1];
            return std::isalnum(static_cast<unsigned char>(preceding)) == 0 && preceding != '_';
        }

        std::string MakeDisplayName(const std::string& rawName)
        {
            std::string trimmed = rawName;
            while (!trimmed.empty() && trimmed.back() == '_')
            {
                trimmed.pop_back();
            }

            if (trimmed.empty())
            {
                return "Property";
            }

            std::string result;
            result.reserve(trimmed.size() * 2);

            char previous = '\0';
            for (char ch : trimmed)
            {
                if (ch == '_')
                {
                    if (!result.empty() && result.back() != ' ')
                    {
                        result.push_back(' ');
                    }
                    previous = ch;
                    continue;
                }

                const bool isUpper = std::isupper(static_cast<unsigned char>(ch)) != 0;
                const bool prevLower = std::islower(static_cast<unsigned char>(previous)) != 0;
                const bool isDigit = std::isdigit(static_cast<unsigned char>(ch)) != 0;

                if (!result.empty() && (isUpper && prevLower))
                {
                    result.push_back(' ');
                }
                else if (!result.empty() && isDigit && std::isdigit(static_cast<unsigned char>(previous)) == 0 && !std::isspace(static_cast<unsigned char>(result.back())))
                {
                    result.push_back(' ');
                }

                result.push_back(ch);
                previous = ch;
            }

            if (!result.empty())
            {
                result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
            }

            return result;
        }
    }

    bool DrawSupportedProperty(const ::Bix::Reflection::PropertyInfo& property, void* instance, const std::string& label)
    {
        if (!property.IsValid())
        {
            return false;
        }

        using PropertyUtils::MatchesType;

        if (MatchesType(property.TypeName, "bool"))
        {
            bool& value = property.Get<bool>(instance);
            return ImGui::Checkbox(label.c_str(), &value);
        }

        if (MatchesType(property.TypeName, "int") || MatchesType(property.TypeName, "int32_t") || MatchesType(property.TypeName, "std::int32_t"))
        {
            int& value = property.Get<int>(instance);
            return DrawDragControl(label.c_str(), value, 1.0f, nullptr, nullptr, "%d");
        }

        if (MatchesType(property.TypeName, "std::int64_t") || MatchesType(property.TypeName, "int64_t"))
        {
            long long& value = property.Get<long long>(instance);
            return DrawDragControl(label.c_str(), value, 1.0f);
        }

        if (MatchesType(property.TypeName, "unsigned int") || MatchesType(property.TypeName, "uint32_t") || MatchesType(property.TypeName, "std::uint32_t"))
        {
            unsigned int& value = property.Get<unsigned int>(instance);
            return DrawDragControl(label.c_str(), value, 1.0f);
        }

        if (MatchesType(property.TypeName, "float"))
        {
            float& value = property.Get<float>(instance);
            return DrawDragControl(label.c_str(), value, 0.1f, nullptr, nullptr, "%.3f");
        }

        if (MatchesType(property.TypeName, "double"))
        {
            double& value = property.Get<double>(instance);
            return DrawDragControl(label.c_str(), value, 0.1f, nullptr, nullptr, "%.3f");
        }

        if (MatchesType(property.TypeName, "Math::Vector2") || MatchesType(property.TypeName, "Vector2"))
        {
            auto& vector = property.Get<BixEngine::Math::Vector2>(instance);
            float values[2] = {vector.x, vector.y};
            if (ImGui::DragFloat2(label.c_str(), values, 0.1f))
            {
                vector.x = values[0];
                vector.y = values[1];
                return true;
            }
            return false;
        }

        if (MatchesType(property.TypeName, "Math::Vector3") || MatchesType(property.TypeName, "Vector3"))
        {
            auto& vector = property.Get<BixEngine::Math::Vector3>(instance);
            float values[3] = {vector.x, vector.y, vector.z};
            if (ImGui::DragFloat3(label.c_str(), values, 0.1f))
            {
                vector.x = values[0];
                vector.y = values[1];
                vector.z = values[2];
                return true;
            }
            return false;
        }

        if (MatchesType(property.TypeName, "Math::Rotator") || MatchesType(property.TypeName, "Rotator"))
        {
            auto& rotator = property.Get<BixEngine::Math::Rotator>(instance);
            float values[3] = {rotator.pitch, rotator.yaw, rotator.roll};
            if (ImGui::DragFloat3(label.c_str(), values, 0.1f))
            {
                rotator.pitch = values[0];
                rotator.yaw = values[1];
                rotator.roll = values[2];
                return true;
            }
            return false;
        }

        if (MatchesType(property.TypeName, "SDL_Color"))
        {
            SDL_Color& color = property.Get<SDL_Color>(instance);
            float values[4] =
            {
                static_cast<float>(color.r) / 255.0f,
                static_cast<float>(color.g) / 255.0f,
                static_cast<float>(color.b) / 255.0f,
                static_cast<float>(color.a) / 255.0f,
            };

            if (ImGui::ColorEdit4(label.c_str(), values))
            {
                color.r = static_cast<Uint8>(std::clamp(values[0], 0.0f, 1.0f) * 255.0f + 0.5f);
                color.g = static_cast<Uint8>(std::clamp(values[1], 0.0f, 1.0f) * 255.0f + 0.5f);
                color.b = static_cast<Uint8>(std::clamp(values[2], 0.0f, 1.0f) * 255.0f + 0.5f);
                color.a = static_cast<Uint8>(std::clamp(values[3], 0.0f, 1.0f) * 255.0f + 0.5f);
                return true;
            }
            return false;
        }

        if (MatchesType(property.TypeName, "String") || MatchesType(property.TypeName, "std::string"))
        {
            thread_local std::array<char, 512> buffer{};

            if (MatchesType(property.TypeName, "String"))
            {
                auto& stringValue = property.Get<BixEngine::String>(instance);
                const std::string current = stringValue.Std();
                const std::size_t copyLength = std::min(buffer.size() - 1, current.size());
                std::memcpy(buffer.data(), current.data(), copyLength);
                buffer[copyLength] = '\0';
                if (ImGui::InputText(label.c_str(), buffer.data(), buffer.size()))
                {
                    stringValue = buffer.data();
                    return true;
                }
                return false;
            }

            auto& stringValue = property.Get<std::string>(instance);
            const std::size_t copyLength = std::min(buffer.size() - 1, stringValue.size());
            std::memcpy(buffer.data(), stringValue.data(), copyLength);
            buffer[copyLength] = '\0';
            if (ImGui::InputText(label.c_str(), buffer.data(), buffer.size()))
            {
                stringValue.assign(buffer.data());
                return true;
            }
            return false;
        }

        return false;
    }

    void DrawUnsupportedProperty(const ::Bix::Reflection::PropertyInfo& property, const std::string& label)
    {
        std::string message = property.TypeName;
        if (!message.empty())
        {
            message += " (read-only)";
        }
        else
        {
            message = "Unsupported";
        }

        Utils::DrawLabelValue(label.c_str(), message, "");
    }

    bool DrawReflectedProperty(const ::Bix::Reflection::PropertyInfo& property, void* instance)
    {
        std::string displayName = PropertyUtils::MakeDisplayName(property.Name);
        if (displayName.empty())
        {
            displayName = property.Name;
        }

        ImGui::PushID(property.Name.c_str());
        const bool handled = DrawSupportedProperty(property, instance, displayName);
        if (!handled)
        {
            DrawUnsupportedProperty(property, displayName);
        }
        ImGui::PopID();
        return handled;
    }

    void GatherClassProperties(const ::Bix::Reflection::ClassInfo& classInfo, std::vector<const ::Bix::Reflection::PropertyInfo*>& outProperties)
    {
        if (classInfo.SuperClass)
        {
            GatherClassProperties(*classInfo.SuperClass, outProperties);
        }

        for (const auto& property : classInfo.Properties)
        {
            outProperties.push_back(&property);
        }
    }

    namespace
    {
        bool AreEquivalent(const ::Bix::Reflection::ClassInfo& lhs, const ::Bix::Reflection::ClassInfo& rhs)
        {
            if (&lhs == &rhs)
            {
                return true;
            }

            if (!lhs.QualifiedName.empty() && !rhs.QualifiedName.empty())
            {
                return lhs.QualifiedName == rhs.QualifiedName;
            }

            if (!lhs.Name.empty() && !rhs.Name.empty())
            {
                return lhs.Name == rhs.Name;
            }

            return false;
        }
    }

    bool IsSubclassOf(const ::Bix::Reflection::ClassInfo& type, const ::Bix::Reflection::ClassInfo& base)
    {
        const ::Bix::Reflection::ClassInfo* current = &type;
        while (current)
        {
            if (AreEquivalent(*current, base))
            {
                return true;
            }

            current = current->SuperClass;
        }

        return false;
    }

    bool DrawClassProperties(const ::Bix::Reflection::ClassInfo& classInfo,
                             void* instance,
                             bool includeHeader,
                             const char* headerLabel,
                             bool showEmptyMessage)
    {
        std::vector<const ::Bix::Reflection::PropertyInfo*> properties;
        properties.reserve(classInfo.Properties.size());
        GatherClassProperties(classInfo, properties);

        if (properties.empty())
        {
            if (includeHeader && headerLabel && headerLabel[0] != '\0')
            {
                Utils::DrawSeparatorText(headerLabel);
            }

            if (showEmptyMessage)
            {
                Utils::DrawEmptyStateMessage("No editable properties.");
            }

            return false;
        }

        if (includeHeader && headerLabel && headerLabel[0] != '\0')
        {
            Utils::DrawSeparatorText(headerLabel);
        }

        bool anyDrawn = false;
        for (const auto* property : properties)
        {
            if (!property)
            {
                continue;
            }

            anyDrawn = DrawReflectedProperty(*property, instance) || anyDrawn;
        }

        return anyDrawn;
    }
}


#include "Gui/Panels/ActorInspector/ReflectionDrawer.h"
#include "Gui/Widgets/Widgets.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Containers/String.h"
#include "SDL3/SDL.h"
#include <array>
#include <cctype>
#include <imgui.h>
#include <string>

#include "ClassInfo.h"
#include "PropertyInfo.h"

namespace BixEngine::Gui::ActorInspector
{
    using namespace Widgets;

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
                else if (!result.empty() && isDigit && std::isdigit(static_cast<unsigned char>(previous)) == 0 && !
                    std::isspace(static_cast<unsigned char>(result.back())))
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

    bool DrawSupportedProperty(const Bix::Reflection::PropertyInfo& property, void* instance, const std::string& label)
    {
        if (!property.IsValid())
            return false;

        using PropertyUtils::MatchesType;

        // ───────────────────────────────────────
        // 🏷️ Affiche une seule ligne "Nom : Type"
        // ───────────────────────────────────────
        std::string typeName = property.TypeName;
        if (!typeName.empty())
        {
            if (typeName.rfind("class ", 0) == 0) typeName.erase(0, 6);
            else if (typeName.rfind("struct ", 0) == 0) typeName.erase(0, 7);
        }

        ImGui::TextUnformatted((label + " : " + typeName).c_str());
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);

        ImGui::PushID(property.Name.c_str());
        bool changed = false;

        // ───────────────────────────────────────
        // 🎛️ Widgets selon le type
        // ───────────────────────────────────────

        if (MatchesType(property.TypeName, "bool"))
        {
            bool& value = property.Get<bool>(instance);
            ImGui::Checkbox("##value", &value);
            return true;
        }

        if (MatchesType(property.TypeName, "int") || MatchesType(property.TypeName, "int32_t") || MatchesType(
            property.TypeName, "std::int32_t"))
        {
            int& value = property.Get<int>(instance);
            changed = DrawDragControl("##value", value, 1.0f, nullptr, nullptr, "%d");
            ImGui::PopID();
            return true;
        }

        if (MatchesType(property.TypeName, "float"))
        {
            float& value = property.Get<float>(instance);
            changed = DrawDragControl("##value", value, 0.1f, nullptr, nullptr, "%.3f");
            ImGui::PopID();
            return true;
        }

        if (MatchesType(property.TypeName, "double"))
        {
            double& value = property.Get<double>(instance);
            changed = DrawDragControl("##value", value, 0.1f, nullptr, nullptr, "%.3f");
            ImGui::PopID();
            return true;
        }

        if (MatchesType(property.TypeName, "Math::Vector2") || MatchesType(property.TypeName, "Vector2"))
        {
            auto& vector = property.Get<Math::Vector2<float>>(instance);
            float values[2] = {vector.x, vector.y};
            if (ImGui::DragFloat2("##value", values, 0.1f))
            {
                vector.x = values[0];
                vector.y = values[1];
            }
            ImGui::PopID();
            return true;
        }

        if (MatchesType(property.TypeName, "Math::Vector3") || MatchesType(property.TypeName, "Vector3"))
        {
            auto& vector = property.Get<Math::Vector3>(instance);
            float values[3] = {vector.x, vector.y, vector.z};
            if (ImGui::DragFloat3("##value", values, 0.1f))
            {
                vector.x = values[0];
                vector.y = values[1];
                vector.z = values[2];
            }
            ImGui::PopID();
            return true;
        }

        if (MatchesType(property.TypeName, "SDL_Color"))
        {
            SDL_Color& color = property.Get<SDL_Color>(instance);
            float values[4] = {color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f};
            if (ImGui::ColorEdit4("##value", values))
            {
                color.r = static_cast<Uint8>(values[0] * 255.f + 0.5f);
                color.g = static_cast<Uint8>(values[1] * 255.f + 0.5f);
                color.b = static_cast<Uint8>(values[2] * 255.f + 0.5f);
                color.a = static_cast<Uint8>(values[3] * 255.f + 0.5f);
            }
            ImGui::PopID();
            return true;
        }

        if (MatchesType(property.TypeName, "String") || MatchesType(property.TypeName, "std::string"))
        {
            thread_local std::array<char, 512> buffer{};

            if (MatchesType(property.TypeName, "String"))
            {
                auto& stringValue = property.Get<String>(instance);
                std::string current = stringValue.Std();
                std::strncpy(buffer.data(), current.c_str(), buffer.size() - 1);
                if (ImGui::InputText("##value", buffer.data(), buffer.size()))
                    stringValue = buffer.data();
            }
            else
            {
                auto& stringValue = property.Get<std::string>(instance);
                std::strncpy(buffer.data(), stringValue.c_str(), buffer.size() - 1);
                if (ImGui::InputText("##value", buffer.data(), buffer.size()))
                    stringValue.assign(buffer.data());
            }

            ImGui::PopID();
            return true;
        }

        // ───────────────────────────────────────
        // 🧩 Si non reconnu
        // ───────────────────────────────────────
        ImGui::PopID();
        return false;
    }


    void DrawUnsupportedProperty(const Bix::Reflection::PropertyInfo& property, const std::string& label)
    {
        std::string message = property.TypeName.empty() ? "Unsupported" : property.TypeName;
        ImGui::TextUnformatted((label + " : " + message).c_str());
    }


    bool DrawReflectedProperty(const Bix::Reflection::PropertyInfo& property, void* instance)
    {
        std::string displayName = PropertyUtils::MakeDisplayName(property.Name);
        if (displayName.empty())
            displayName = property.Name;

        ImGui::PushID(property.Name.c_str());
        bool handled = DrawSupportedProperty(property, instance, displayName);
        ImGui::PopID();

        if (!handled)
            DrawUnsupportedProperty(property, displayName);

        // ✅ Correctif ici
        ImGui::Dummy(ImVec2(0.0f, 8.0f));

        return handled;
    }


    void GatherClassProperties(const Bix::Reflection::ClassInfo& classInfo,
                               std::vector<const Bix::Reflection::PropertyInfo*>& outProperties)
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
        bool AreEquivalent(const Bix::Reflection::ClassInfo& lhs, const Bix::Reflection::ClassInfo& rhs)
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

    bool IsSubclassOf(const Bix::Reflection::ClassInfo& type, const Bix::Reflection::ClassInfo& base)
    {
        const Bix::Reflection::ClassInfo* current = &type;
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

    bool DrawClassProperties(const Bix::Reflection::ClassInfo& classInfo, void* instance, bool includeHeader,
                             const char* headerLabel, bool showEmptyMessage)
    {
        std::vector<const Bix::Reflection::PropertyInfo*> properties;
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

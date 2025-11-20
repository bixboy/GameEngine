#include "Gui/Panels/ActorInspector/PropertyInspector.h"

#include <array>
#include <cstdio>
#include <format>
#include <cmath>

#include "Gui/Utils/GuiHelpers.h"
#include "Gui/Widgets/Widgets.h"
#include "Gui/Widgets/Controls/DrawVector3Control.h"

#include "Math/Vector2.h"
#include "Math/Vector3.h"

#include "SDL3/SDL.h"
#include "ClassInfo.h"
#include "PropertyInfo.h"
#include "Gui/Utils/ExposedVariableUtils.h"
#include "Ressources/Texture.h"


namespace BixEngine::Gui::ActorInspector
{

    // ============================================================================
    //  DrawSupportedProperty
    // ============================================================================
    bool PropertyInspector::DrawSupportedProperty(const Bix::Reflection::PropertyInfo& property, void* instance, const std::string& label)
    {
        if (!property.IsValid())
            return false;

        std::string typeName = ExposedVariableUtils::CleanTypeName(property.TypeName);
        ImGui::TextUnformatted((label + " : " + typeName).c_str());
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);


        // ---------------------------------------------------------------------
        // bool
        // ---------------------------------------------------------------------
        if (ExposedVariableUtils::MatchesType(property.TypeName, "bool"))
        {
            bool& value = property.Get<bool>(instance);
            ImGui::Checkbox("##value", &value);
            return true;
        }

        // ---------------------------------------------------------------------
        // int
        // ---------------------------------------------------------------------
        if (ExposedVariableUtils::MatchesType(property.TypeName, "int") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "int32_t") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "std::int32_t"))
        {
            int& v = property.Get<int>(instance);
            Widgets::DrawDragControl("##value", v, 1.0f, nullptr, nullptr, "%d");
            return true;
        }

        // ---------------------------------------------------------------------
        // float
        // ---------------------------------------------------------------------
        if (ExposedVariableUtils::MatchesType(property.TypeName, "float"))
        {
            float& v = property.Get<float>(instance);
            Widgets::DrawDragControl("##value", v, 0.1f, nullptr, nullptr, "%.3f");
            return true;
        }

        // ---------------------------------------------------------------------
        // double
        // ---------------------------------------------------------------------
        if (ExposedVariableUtils::MatchesType(property.TypeName, "double"))
        {
            double& v = property.Get<double>(instance);
            Widgets::DrawDragControl("##value", v, 0.1f, nullptr, nullptr, "%.3f");
            return true;
        }

        // ---------------------------------------------------------------------
        // Vector2
        // ---------------------------------------------------------------------
        if (ExposedVariableUtils::MatchesType(property.TypeName, "Math::Vector2") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "Vector2"))
        {
            auto& vec = property.Get<Math::Vector2<float>>(instance);

            float vals[2] = { vec.x, vec.y };

            if (ImGui::DragFloat2("##value", vals, 0.1f))
            {
                vec.x = vals[0];
                vec.y = vals[1];
            }
            return true;
        }

        // ---------------------------------------------------------------------
        // Vector3
        // ---------------------------------------------------------------------
        if (ExposedVariableUtils::MatchesType(property.TypeName, "Math::Vector3") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "Vector3"))
        {
            auto& vec = property.Get<Math::Vector3>(instance);

            float vals[3] = { vec.x, vec.y, vec.z };

            if (Widgets::DrawVector3Control("##value", vals, 0.0f, 0.1f, "%.3f"))
            {
                vec.x = vals[0];
                vec.y = vals[1];
                vec.z = vals[2];
            }
            return true;
        }

        // ---------------------------------------------------------------------
        // SDL_Color
        // ---------------------------------------------------------------------
        if (ExposedVariableUtils::MatchesType(property.TypeName, "SDL_Color"))
        {
            SDL_Color& c = property.Get<SDL_Color>(instance);

            float vals[4] =
            {
                c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f
            };

            if (!ImGui::ColorEdit4("##value", vals))
                return false;
    
            c.r = static_cast<Uint8>(std::lround(vals[0] * 255.f));
            c.g = static_cast<Uint8>(std::lround(vals[1] * 255.f));
            c.b = static_cast<Uint8>(std::lround(vals[2] * 255.f));
            c.a = static_cast<Uint8>(std::lround(vals[3] * 255.f));

            return true;
        }

        // ---------------------------------------------------------------------
        // string
        // ---------------------------------------------------------------------
        if (ExposedVariableUtils::MatchesType(property.TypeName, "String") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "std::string"))
        {
            thread_local std::array<char, 512> buffer;

            if (ExposedVariableUtils::MatchesType(property.TypeName, "String"))
            {
                auto& s = property.Get<String>(instance);

                strncpy_s(buffer.data(), buffer.size(), s.c_str(), buffer.size() - 1);
                buffer.back() = '\0';

                if (ImGui::InputText("##value", buffer.data(), buffer.size()))
                    s = buffer.data();
            }
            else
            {
                auto& s = property.Get<std::string>(instance);

                strncpy_s(buffer.data(), buffer.size(), s.c_str(), buffer.size() - 1);
                buffer.back() = '\0';

                if (ImGui::InputText("##value", buffer.data(), buffer.size()))
                    s.assign(buffer.data());
            }
            return true;
        }

        // ---------------------------------------------------------------------
        // Texture* (BixEngine::resources::Texture*)
        // ---------------------------------------------------------------------
        if (ExposedVariableUtils::MatchesType(property.TypeName, "Texture") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "BixEngine::resources::Texture") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "resources::Texture") || // Ajout pour être sûr
            ExposedVariableUtils::MatchesType(property.TypeName, "Texture*"))
        {
            // On récupère la référence vers le pointeur de texture
            resources::Texture*& tex = property.Get<resources::Texture*>(instance);

            // Style pour la preview
            float previewSize = 64.0f;
            ImVec2 sizeVec(previewSize, previewSize);

            ImGui::PushID("TextureProperty");

            // Colonnes pour aligner l'image et le texte/boutons
            ImGui::BeginGroup();

            if (tex == nullptr)
            {
                // Case vide : Bouton pour dire "Pas de texture"
                ImGui::Button("Null (Drag Here)", sizeVec);
            }
            else
            {
                // Case remplie : Affichage de l'image
                // NOTE: Avec le backend SDL_Renderer d'ImGui, on cast SDL_Texture* en ImTextureID
                ImTextureID imgID = (ImTextureID)tex->GetNativeHandle();
                
                // On dessine l'image. On peut ajouter une bordure (dernier param = border_col)
                ImGui::Image(imgID, sizeVec, ImVec2(0,0), ImVec2(1,1), ImVec4(1,1,1,1), ImVec4(1,1,1,0.5f));
                
                // Tooltip au survol de l'image (Zoom)
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Dimensions: %dx%d", tex->GetWidth(), tex->GetHeight());
                    ImGui::Text("Format: %d", (int)tex->GetFormat());
                    float zoomSize = 256.0f;
                    ImGui::Image(imgID, ImVec2(zoomSize, zoomSize));
                    ImGui::EndTooltip();
                }
            }

            // -------------------------------------------------------------
            // DRAG & DROP TARGET (Pour assigner depuis l'Asset Browser)
            // -------------------------------------------------------------
            if (ImGui::BeginDragDropTarget())
            {
                // "ASSET_TEXTURE" est l'ID que tu devras utiliser dans ton Asset Browser quand tu fais le Drag
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_TEXTURE"))
                {
                    const wchar_t* path = (const wchar_t*)payload->Data;
                    
                    // TODO: Appeler ton ResourceManager ici !
                    // Exemple : tex = BixEngine::ResourceManager::Get().LoadTexture(path);
                    // Pour l'instant on log juste
                    printf("Texture dropped path (TODO implement load): %ls\n", path);
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::SameLine();

            // Infos textuelles et boutons à droite de l'image
            ImGui::BeginGroup();
            if (tex != nullptr)
            {
                // Nom de la texture (Si IResource a un nom, sinon on met "Texture")
                // ImGui::Text("%s", tex->GetName().c_str()); 
                ImGui::Text("Res: %dx%d", tex->GetWidth(), tex->GetHeight());
                
                if (ImGui::Button("Remove"))
                {
                    tex = nullptr;
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No Texture Assigned");
            }
            ImGui::EndGroup();

            ImGui::EndGroup();
            ImGui::PopID();

            return true;
        }
        
        return false;
    }


    // ============================================================================
    //  DrawReflectedProperty
    // ============================================================================
    bool PropertyInspector::DrawReflectedProperty(const Bix::Reflection::PropertyInfo& property, void* instance)
    {
        const std::string label = ExposedVariableUtils::MakeDisplayName(property.Name);

        ImGui::PushID(property.Name.c_str());
        bool handled = DrawSupportedProperty(property, instance, label);
        ImGui::PopID();

        if (!handled)
        {
            const std::string typeName = ExposedVariableUtils::CleanTypeName(property.TypeName);
        }

        ImGui::Dummy(ImVec2(0.f, 8.f));
        return handled;
    }


    // ============================================================================
    //  GatherClassProperties
    // ============================================================================
    void PropertyInspector::GatherClassProperties(const Bix::Reflection::ClassInfo& cls, std::vector<const Bix::Reflection::PropertyInfo*>& out)
    {
        if (cls.SuperClass)
            GatherClassProperties(*cls.SuperClass, out);

        for (const auto& p : cls.Properties)
            out.push_back(&p);
    }


    // ============================================================================
    //  DrawClassProperties
    // ============================================================================
    bool PropertyInspector::DrawClassProperties(const Bix::Reflection::ClassInfo& classInfo, void* instance, bool includeHeader, const char* headerLabel, bool showEmptyMessage)
    {
        std::vector<const Bix::Reflection::PropertyInfo*> props;
        props.reserve(classInfo.Properties.size());
        GatherClassProperties(classInfo, props);

        if (props.empty())
        {
            if (includeHeader && headerLabel && headerLabel[0])
                Utils::DrawSeparatorText(headerLabel);

            if (showEmptyMessage)
                Utils::DrawEmptyStateMessage("No editable properties.");

            return false;
        }

        if (includeHeader && headerLabel && headerLabel[0])
            Utils::DrawSeparatorText(headerLabel);

        bool any = false;

        for (auto* p : props)
        {
            if (!p)
                continue;

            any = DrawReflectedProperty(*p, instance) || any;
        }

        return any;
    }


    // ============================================================================
    //  DrawExposedVariablesSection
    // ============================================================================
    void PropertyInspector::DrawExposedVariablesSection(BaseAssetEditorController::SharedState& state, std::string_view sectionLabel, const char* emptyMessage)
    {
        const std::string header = sectionLabel.empty() ? "Variables" : std::string(sectionLabel);
        Utils::DrawSeparatorText(header.c_str());

        if (state.exposedVariables.empty())
        {
            Utils::DrawEmptyStateMessage(emptyMessage ? emptyMessage : "No entries to display.");
            return;
        }

        ImGui::Spacing();

        for (size_t i = 0; i < state.exposedVariables.size(); ++i)
        {
            auto& var = state.exposedVariables[i];
            ImGui::PushID(static_cast<int>(i));

            std::string typeName = ExposedVariableUtils::CleanTypeName(var.type.Std());
            std::string displayName = ExposedVariableUtils::MakeDisplayName(var.name.Std());

            if (displayName.empty())
                displayName = var.name.Std().empty() ? "Property" : var.name.Std();

            ImGui::TextUnformatted((displayName + " : " + (typeName.empty() ? "Unknown" : typeName)).c_str());
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);

            std::string cleaned = ExposedVariableUtils::TrimBraces(var.value.Std());
            auto nums = ExposedVariableUtils::ExtractNumbers(cleaned);

            bool handled = false;

            // bool
            if (ExposedVariableUtils::MatchesType(typeName, "bool"))
            {
                bool value = (cleaned == "true" || cleaned == "1");

                if (ImGui::Checkbox("##Value", &value))
                    var.value = value ? "true" : "false";

                handled = true;
            }
            // int
            else if (ExposedVariableUtils::MatchesType(typeName, "int") || ExposedVariableUtils::MatchesType(typeName, "int32_t"))
            {
                int v = nums.empty() ? 0 : static_cast<int>(nums.front());

                if (Widgets::DrawDragControl("##Value", v, 1.0f))
                    var.value = std::to_string(v);

                handled = true;
            }
            // float
            else if (ExposedVariableUtils::MatchesType(typeName, "float"))
            {
                float v = nums.empty() ? 0.f : nums.front();

                if (Widgets::DrawDragControl("##Value", v, 0.1f))
                {
                    char buf[64];
                    std::snprintf(buf, sizeof(buf), "%.3ff", v);
                    var.value = buf;
                }

                handled = true;
            }
            // vec2
            else if (ExposedVariableUtils::MatchesType(typeName, "Vector2") ||
                     ExposedVariableUtils::MatchesType(typeName, "Math::Vector2"))
            {
                float v[2] =
                {
                    nums.size() > 0 ? nums[0] : 0.f,
                    nums.size() > 1 ? nums[1] : 0.f
                };

                if (ImGui::DragFloat2("##Value", v, 0.1f))
                    var.value = std::format("{{{:.3f}f, {:.3f}f}}", v[0], v[1]);

                handled = true;
            }
            // vec3
            else if (ExposedVariableUtils::MatchesType(typeName, "Vector3") ||
                     ExposedVariableUtils::MatchesType(typeName, "Math::Vector3"))
            {
                float v[3] =
                {
                    nums.size() > 0 ? nums[0] : 0.f,
                    nums.size() > 1 ? nums[1] : 0.f,
                    nums.size() > 2 ? nums[2] : 0.f
                };

                if (Widgets::DrawVector3Control("##Value", v, 0.f, 0.1f, "%.3f"))
                    var.value = std::format("{{{:.3f}f, {:.3f}f, {:.3f}f}}", v[0], v[1], v[2]);

                handled = true;
            }

            if (!handled)
            {
                char buffer[256];
                strncpy_s(buffer, cleaned.c_str(), sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';

                if (ImGui::InputText("##Value", buffer, sizeof(buffer)))
                    var.value = buffer;
            }

            ImGui::Dummy(ImVec2(0, 8));
            ImGui::PopID();
        }
    }

}

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
#include "Ressources/ResourceManager.h"
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

// N'oublie pas d'inclure ton manager en haut du fichier
#include "Ressources/ResourceManager.h" 
// Si tu veux scanner le dossier assets plus tard :
#include <filesystem> 

// ... (Le début de ta fonction DrawSupportedProperty) ...

        // ---------------------------------------------------------------------
        // Texture* // ---------------------------------------------------------------------
        if (ExposedVariableUtils::MatchesType(property.TypeName, "Texture") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "BixEngine::resources::Texture") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "resources::Texture") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "Texture*"))
        {
            resources::Texture*& tex = property.Get<resources::Texture*>(instance);

            ImGui::PushID(property.Name.c_str());
            ImGui::BeginGroup();

            // --- PREVIEW ---
            float previewSize = 64.0f;
            ImVec2 sizeVec(previewSize, previewSize);
            bool openSelector = false;

            if (tex == nullptr)
            {
                if (ImGui::Button("Empty\n(Click)", sizeVec)) openSelector = true;
            }
            else
            {
                ImTextureID imgID = (ImTextureID)tex->GetNativeHandle();
                if (ImGui::ImageButton("##TextureBtn", imgID, sizeVec, ImVec2(0,0), ImVec2(1,1), ImVec4(0,0,0,1)))
                    openSelector = true;

                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Size: %dx%d", tex->GetWidth(), tex->GetHeight());
                    ImGui::Image(imgID, ImVec2(256, 256));
                    ImGui::EndTooltip();
                }
            }

            if (openSelector) ImGui::OpenPopup("TextureSelectorPopup");

            ImGui::SameLine();

            // --- BOUTONS D'ACTION ---
            ImGui::BeginGroup();
            if (tex)
            {
                ImGui::Text("Res: %dx%d", tex->GetWidth(), tex->GetHeight());
                if (ImGui::Button("Clear")) tex = nullptr;
            }
            else
            {
                ImGui::TextDisabled("No Texture");
            }
            ImGui::EndGroup();

            // --- POPUP LISTE DES TEXTURES ---
            ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
            if (ImGui::BeginPopup("TextureSelectorPopup"))
            {
                ImGui::TextDisabled("Select Texture");
                ImGui::Separator();

                // 1. Option "None"
                if (ImGui::Selectable("None"))
                {
                    tex = nullptr;
                    ImGui::CloseCurrentPopup();
                }

                // 2. Récupérer les textures chargées via le ResourceManager
                auto loadedKeys = resources::ResourceManager::Get().GetLoadedResourceKeys<resources::Texture>();

                if (loadedKeys.empty())
                {
                    ImGui::TextColored(ImVec4(1,0.5f,0,1), "No textures loaded in cache.");
                }

                for (const auto& path : loadedKeys)
                {
                    bool isSelected = (tex != nullptr && path == "TODO_COMPARE_PATH_IF_POSSIBLE"); 
                    if (ImGui::Selectable(path.c_str(), isSelected))
                    {
                        auto sharedTex = resources::ResourceManager::Get().Get<resources::Texture>(path);
                        tex = sharedTex.get();
                        
                        ImGui::CloseCurrentPopup();
                    }
                }
                
                // --- OPTIONNEL : SCAN DU DISQUE ---
                // Si tu veux voir les fichiers qui ne sont PAS encore chargés :
                /*
                ImGui::SeparatorText("Assets on Disk");
                namespace fs = std::filesystem;
                if (fs::exists("assets/")) {
                    for (const auto& entry : fs::recursive_directory_iterator("assets/")) {
                        if (entry.path().extension() == ".png" || entry.path().extension() == ".jpg") {
                            std::string p = entry.path().string();
                            if (ImGui::Selectable(p.c_str())) {
                                auto sharedTex = resources::ResourceManager::Get().Get<resources::Texture>(p.c_str());
                                tex = sharedTex.get();
                            }
                        }
                    }
                }
                */

                ImGui::EndPopup();
            }

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

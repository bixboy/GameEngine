#include "Gui/Panels/ActorInspector/PropertyInspector.h"

#include <array>
#include <cstdio>
#include <format>
#include <cmath>
#include <imgui.h>
#include <map>
#include <vector>

#include "Gui/Utils/GuiHelpers.h"
#include "Gui/Widgets/Widgets.h"
#include "Gui/Widgets/Controls/DrawVector3Control.h"

#include "Math/Vector2.h"
#include "Math/Vector3.h"

#include "SDL3/SDL.h"
#include "Core/ClassInfo.h"
#include "Core/PropertyInfo.h"
#include "Gui/Utils/ExposedVariableUtils.h"
#include "Ressources/Core/ResourceManager.h"
#include "Ressources/RessourcesClass/Texture.h"
#include "Ressources/RessourcesClass/AudioClip.h"
#include "Ressources/RessourcesClass/AudioContainer.h"
#include "Ressources/RessourcesClass/SpriteAtlas.h"
#include "Utils/FileIO/FilesUtils.h"
#include "Gui/Utils/ContentBrowserUtils.h"


namespace BixEngine::Gui::ActorInspector
{

    // ============================================================================
    //  Helper: SimplifyTypeName
    // ============================================================================
    static std::string SimplifyTypeName(std::string typeName)
    {
        // Remove common namespaces
        const std::array<std::string, 6> namespaces = { 
            "BixEngine::", "Game::", "Math::", "resources::", "std::", "Graphics::" 
        };

        for (const auto& ns : namespaces)
        {
            size_t pos;
            while ((pos = typeName.find(ns)) != std::string::npos)
            {
                typeName.erase(pos, ns.length());
            }
        }

        // Handle templates like shared_ptr<T> -> T
        if (typeName.find("shared_ptr<") != std::string::npos)
        {
            size_t start = typeName.find('<') + 1;
            size_t end = typeName.find_last_of('>');
            if (end > start)
                typeName = typeName.substr(start, end - start);
        }
        
        // Handle Vector2<float> -> Vector2
        if (typeName.find("Vector2<") != std::string::npos)
        {
             typeName = "Vector2";
        }

        // Remove pointers/refs
        typeName.erase(std::remove(typeName.begin(), typeName.end(), '*'), typeName.end());
        typeName.erase(std::remove(typeName.begin(), typeName.end(), '&'), typeName.end());

        return typeName;
    }

    // ============================================================================
    //  DrawSupportedProperty
    // ============================================================================

    bool DrawStringArray(const Bix::Reflection::PropertyInfo& property, void* instance)
    {
        auto& list = property.Get<std::vector<String>>(instance);

        if (ImGui::TreeNode(property.Name.c_str()))
        {
            // Add Button
            if (ImGui::Button("Add Element"))
            {
                list.emplace_back();
            }

            ImGui::SameLine();
            ImGui::TextDisabled("(%zu elements)", list.size());

            // List Elements
            for (size_t i = 0; i < list.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                
                // Remove Button
                if (ImGui::Button("X"))
                {
                    list.erase(list.begin() + i);
                    ImGui::PopID();
                    continue;
                }
                ImGui::SameLine();

                char buffer[260];
                strncpy_s(buffer, sizeof(buffer), list[i].c_str(), _TRUNCATE);
                
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::InputText("##Val", buffer, sizeof(buffer)))
                {
                    list[i] = buffer;
                }

                // Drag & Drop Target
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                    {
                        const char* path = (const char*)payload->Data;
                        std::string fullPath = path;
                        std::replace(fullPath.begin(), fullPath.end(), '\\', '/');
                        list[i] = fullPath.c_str(); 
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::PopID();
            }

            ImGui::TreePop();
        }
        return true;
    }

    // ============================================================================
    //  DrawTSubclassOfArray
    // ============================================================================
    bool DrawTSubclassOfArray(const Bix::Reflection::PropertyInfo& property, void* instance)
    {
        // TArray<TSubclassOf<T>> layout is vector<String>
        auto& list = property.Get<std::vector<String>>(instance);

        if (ImGui::TreeNode(property.Name.c_str()))
        {
            // Add Button
            if (ImGui::Button("Add Element"))
            {
                list.emplace_back();
            }

            ImGui::SameLine();
            ImGui::TextDisabled("(%zu elements)", list.size());

            // List Elements
            for (size_t i = 0; i < list.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                
                // Remove Button
                if (ImGui::Button("X"))
                {
                    list.erase(list.begin() + i);
                    ImGui::PopID();
                    continue; 
                }
                ImGui::SameLine();

                // Asset Selector
                // Display current value as a button
                String currentPath = list[i];
                std::string display = "None";
                if (!currentPath.IsEmpty())
                {
                    display = std::filesystem::path(currentPath.c_str()).filename().string();
                }

                if (ImGui::Button(display.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                {
                    ImGui::OpenPopup("AssetSelectorPopup");
                }

                // Drag & Drop (keep existing support)
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                    {
                        const char* path = (const char*)payload->Data;
                        std::string fullPath = path;
                        std::replace(fullPath.begin(), fullPath.end(), '\\', '/');
                        list[i] = fullPath.c_str(); 
                    }
                    ImGui::EndDragDropTarget();
                }

                // Popup
                if (ImGui::BeginPopup("AssetSelectorPopup"))
                {
                    ImGui::TextDisabled("Select Prefab");
                    ImGui::Separator();
                    
                    if (ImGui::Selectable("None"))
                    {
                        list[i] = "";
                        ImGui::CloseCurrentPopup();
                    }

                    // Scan for .prefab files
                    // Assuming generic TSubclassOf is mostly used for prefabs in this context
                    static std::vector<std::string> extensions = { ".prefab" };
                    auto foundFiles = FilesUtils::Utilities::ScanDirectory("", extensions);

                    static char searchBuffer[128] = "";
                    ImGui::InputTextWithHint("##Search", "Search...", searchBuffer, sizeof(searchBuffer));

                    for (const auto& path : foundFiles)
                    {
                        std::string pathStr = path.string();
                        std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

                        std::string filename = path.filename().string();

                        if (searchBuffer[0] != '\0')
                        {
                            // Simple substring search
                            std::string searchS = searchBuffer;
                            // TODO: Case insensitive
                            if (filename.find(searchS) == std::string::npos) 
                                continue;
                        }

                        if (ImGui::Selectable(filename.c_str(), currentPath == pathStr.c_str()))
                        {
                            list[i] = pathStr.c_str();
                            ImGui::CloseCurrentPopup();
                        }
                        
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("%s", pathStr.c_str());
                    }

                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }

            ImGui::TreePop();
        }
        return true;
    }



    // ============================================================================
    //  DrawTSubclassOf (Single)
    // ============================================================================
    bool DrawTSubclassOf(const Bix::Reflection::PropertyInfo& property, void* instance)
    {
        // TSubclassOf<T> layout is String (asset path)
        auto& pathStr = property.Get<String>(instance);

        // Display current value as a button
        String currentPath = pathStr;
        std::string display = "None";
        if (!currentPath.IsEmpty())
        {
            display = std::filesystem::path(currentPath.c_str()).filename().string();
        }

        if (ImGui::Button(display.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        {
            ImGui::OpenPopup("AssetSelectorPopup_Single");
        }

        // Drag & Drop
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                const char* draggedPath = (const char*)payload->Data;
                std::string fullPath = draggedPath;
                std::replace(fullPath.begin(), fullPath.end(), '\\', '/');
                pathStr = fullPath.c_str(); 
            }
            ImGui::EndDragDropTarget();
        }

        // Popup
        if (ImGui::BeginPopup("AssetSelectorPopup_Single"))
        {
             ImGui::TextDisabled("Select Prefab");
             ImGui::Separator();
             
             if (ImGui::Selectable("None"))
             {
                 pathStr = "";
                 ImGui::CloseCurrentPopup();
             }

             // Scan for .prefab files
             static std::vector<std::string> extensions = { ".prefab" };
             auto foundFiles = FilesUtils::Utilities::ScanDirectory("", extensions);

             static char searchBuffer[128] = "";
             ImGui::InputTextWithHint("##Search", "Search...", searchBuffer, sizeof(searchBuffer));

             for (const auto& path : foundFiles)
             {
                 std::string pStr = path.string();
                 std::replace(pStr.begin(), pStr.end(), '\\', '/');
                 std::string filename = path.filename().string();

                 if (searchBuffer[0] != '\0')
                 {
                     std::string searchS = searchBuffer;
                     // TODO: Case insensitive
                     if (filename.find(searchS) == std::string::npos) 
                         continue;
                 }

                 if (ImGui::Selectable(filename.c_str(), currentPath == pStr.c_str()))
                 {
                     pathStr = pStr.c_str();
                     ImGui::CloseCurrentPopup();
                 }
                 
                 if (ImGui::IsItemHovered())
                     ImGui::SetTooltip("%s", pStr.c_str());
             }

             ImGui::EndPopup();
        }
        return true;
    }

    bool PropertyInspector::DrawSupportedProperty(const Bix::Reflection::PropertyInfo& property, void* instance, const std::string& /*label*/)
    {
        if (!property.IsValid())
            return false;

        std::string typeName = SimplifyTypeName(property.TypeName);
        
        // --- Generic Resource Drawer Registry ---
        using DrawerFunc = std::function<bool(const Bix::Reflection::PropertyInfo&, void*)>;
        static const std::unordered_map<std::string, DrawerFunc> ResourceDrawers = 
        {
            { "AudioClip", [](const auto& p, void* i) { return DrawSharedResourceProperty<resources::AudioClip>(p, i, { ".mp3", ".wav", ".ogg" }); } },
            { "AudioContainer", [](const auto& p, void* i) { return DrawSharedResourceProperty<resources::AudioContainer>(p, i, { ".bixaudio" }); } },
            { "SpriteAtlas", [](const auto& p, void* i) { return DrawSharedResourceProperty<resources::SpriteAtlas>(p, i, { ".atlas" }); } },
            // Texture is handled specifically below because it has a custom preview widget
        };

        // Check registry first
        auto it = ResourceDrawers.find(typeName);
        if (it != ResourceDrawers.end())
        {
            return it->second(property, instance);
        }

        if (ExposedVariableUtils::MatchesType(property.TypeName, "bool"))
            return DrawBool(property, instance);

        if (ExposedVariableUtils::MatchesType(property.TypeName, "int") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "int32_t") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "std::int32_t"))
            return DrawInt(property, instance);

        if (ExposedVariableUtils::MatchesType(property.TypeName, "float"))
            return DrawFloat(property, instance);

        if (ExposedVariableUtils::MatchesType(property.TypeName, "double"))
            return DrawDouble(property, instance);

        if (ExposedVariableUtils::MatchesType(property.TypeName, "Math::Vector2") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "Vector2") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "Math::Vector2<float>") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "BixEngine::Math::Vector2<float>"))
            return DrawVector2(property, instance);

        if (ExposedVariableUtils::MatchesType(property.TypeName, "Math::Vector3") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "Vector3"))
            return DrawVector3(property, instance);

        if (ExposedVariableUtils::MatchesType(property.TypeName, "SDL_Color"))
            return DrawColor(property, instance);

        if (ExposedVariableUtils::MatchesType(property.TypeName, "String") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "std::string"))
            return DrawString(property, instance);
        
        if (property.TypeName.find("vector<") != std::string::npos && 
           (property.TypeName.find("String") != std::string::npos || property.TypeName.find("string") != std::string::npos))
        {
             return DrawStringArray(property, instance);
        }

        // TArray<TSubclassOf<T>> (or vector<TSubclassOf<T>>)
        // We detect "TSubclassOf" in the type name.
        if ((property.TypeName.find("vector<") != std::string::npos || property.TypeName.find("TArray<") != std::string::npos) && 
            property.TypeName.find("TSubclassOf<") != std::string::npos)
        {
             // We cast to vector<String> assuming TSubclassOf has same layout as String
             // This is a safe assumption given our implementation of TSubclassOf
             return DrawTSubclassOfArray(property, instance);
        }

        // Single TSubclassOf<T>
        if (property.TypeName.find("TSubclassOf<") != std::string::npos)
        {
            return DrawTSubclassOf(property, instance);
        }

        if (ExposedVariableUtils::MatchesType(property.TypeName, "Texture") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "BixEngine::resources::Texture") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "resources::Texture") ||
            ExposedVariableUtils::MatchesType(property.TypeName, "Texture*"))
            return DrawTexture(property, instance);

        return false;
    }

    template <typename T>
    bool PropertyInspector::DrawSharedResourceProperty(const Bix::Reflection::PropertyInfo& property, void* instance, const std::vector<std::string>& extensions)
    {
        std::shared_ptr<T>& resourcePtr = property.Get<std::shared_ptr<T>>(instance);

        ImGui::PushID(property.Name.c_str());
        
        const std::filesystem::path root = Gui::ContentBrowserUtils::GetContentRoot();
        std::string currentPath = resourcePtr ? resourcePtr->GetPath().ToStdString() : "";
        std::string previewStr = resourcePtr ? FilesUtils::Utilities::ExtractDisplayName(resourcePtr->GetPath().ToStdString()).Std() : "None";

        float availableWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(availableWidth);

        if (ImGui::BeginCombo("##ResourceCombo", previewStr.c_str()))
        {
            if (ImGui::Selectable("None", !resourcePtr))
            {
                resourcePtr.reset();
            }

            std::vector<std::filesystem::path> files = FilesUtils::Utilities::ScanDirectory(root, extensions);
            
            for (const auto& file : files)
            {
                std::string display = FilesUtils::Utilities::ExtractDisplayName(file).Std();
                bool selected = (currentPath == file.generic_string());
                
                if (ImGui::Selectable(display.c_str(), selected))
                {
                    resourcePtr = resources::ResourceManager::Get().Get<T>(file.generic_string());
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::PopID();
        return true;
    }

    bool PropertyInspector::DrawBool(const Bix::Reflection::PropertyInfo& property, void* instance)
    {
        bool& value = property.Get<bool>(instance);
        ImGui::Checkbox("##value", &value);
        return true;
    }

    bool PropertyInspector::DrawInt(const Bix::Reflection::PropertyInfo& property, void* instance)
    {
        int& v = property.Get<int>(instance);
        Widgets::DrawDragControl("##value", v, 1.0f, nullptr, nullptr, "%d");
        return true;
    }

    bool PropertyInspector::DrawFloat(const Bix::Reflection::PropertyInfo& property, void* instance)
    {
        float& v = property.Get<float>(instance);
        Widgets::DrawDragControl("##value", v, 0.1f, nullptr, nullptr, "%.3f");
        return true;
    }

    bool PropertyInspector::DrawDouble(const Bix::Reflection::PropertyInfo& property, void* instance)
    {
        double& v = property.Get<double>(instance);
        Widgets::DrawDragControl("##value", v, 0.1f, nullptr, nullptr, "%.3f");
        return true;
    }

    bool PropertyInspector::DrawVector2(const Bix::Reflection::PropertyInfo& property, void* instance)
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

    bool PropertyInspector::DrawVector3(const Bix::Reflection::PropertyInfo& property, void* instance)
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

    bool PropertyInspector::DrawColor(const Bix::Reflection::PropertyInfo& property, void* instance)
    {
        SDL_Color& c = property.Get<SDL_Color>(instance);
        float vals[4] = { c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f };

        if (!ImGui::ColorEdit4("##value", vals))
            return false;

        c.r = static_cast<Uint8>(std::lround(vals[0] * 255.f));
        c.g = static_cast<Uint8>(std::lround(vals[1] * 255.f));
        c.b = static_cast<Uint8>(std::lround(vals[2] * 255.f));
        c.a = static_cast<Uint8>(std::lround(vals[3] * 255.f));
        return true;
    }

    bool PropertyInspector::DrawString(const Bix::Reflection::PropertyInfo& property, void* instance)
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

    bool PropertyInspector::DrawTexture(const Bix::Reflection::PropertyInfo& property, void* instance)
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
            ImTextureID imgID = reinterpret_cast<ImTextureID>(tex->GetNativeHandle());
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

        // --- ACTION BUTTONS ---
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

        // --- TEXTURE SELECTOR POPUP ---
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        if (ImGui::BeginPopup("TextureSelectorPopup"))
        {
            static char searchBuffer[128] = "";
            ImGui::TextDisabled("Select Texture");
            ImGui::Separator();

            if (ImGui::Selectable("None"))
            {
                tex = nullptr;
                ImGui::CloseCurrentPopup();
            }

            auto foundFiles = FilesUtils::Utilities::ScanDirectory("", { ".png", ".jpg", ".jpeg", ".tga", ".bmp" });

            if (foundFiles.empty())
            {
                ImGui::TextDisabled("No textures found.");
            }

            for (const auto& path : foundFiles)
            {
                std::string pathStr = path.string();
                std::replace(pathStr.begin(), pathStr.end(), '\\', '/'); 

                if (searchBuffer[0] != '\0')
                {
                    if (pathStr.find(searchBuffer) == std::string::npos)
                        continue;
                }

                std::string displayName = path.filename().string();

                if (ImGui::Selectable(displayName.c_str()))
                {
                    auto sharedTex = resources::ResourceManager::Get().Get<resources::Texture>(pathStr);
                    if (sharedTex)
                        tex = sharedTex.get();
        
                    ImGui::CloseCurrentPopup();
                }

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", pathStr.c_str());
            }

            ImGui::EndPopup();
        }

        ImGui::EndGroup();
        ImGui::PopID();
        return true;
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

        // Debug info if not handled?
        /*
        if (!handled)
        {
            const std::string typeName = ExposedVariableUtils::CleanTypeName(property.TypeName);
        }
        */

        if (handled)
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

        // Group by Category
        std::map<std::string, std::vector<const Bix::Reflection::PropertyInfo*>> categories;
        std::vector<const Bix::Reflection::PropertyInfo*> uncategorized;
        std::vector<std::string> processedNames;

        for (auto* p : props)
        {
            if (!p) continue;
            
            // Deduplicate by name
            bool alreadyExists = false;
            for (const auto& existing : processedNames)
            {
                if (existing == p->Name) 
                {
                    alreadyExists = true;
                    break;
                }
            }
            if (alreadyExists) continue;
            processedNames.push_back(p->Name);

            // Visibility Logic
            bool isPublic = p->GetMetadata("Access") == "Public";
            bool hasEditAnywhere = p->HasMetadata("EditAnywhere");
            bool hasHide = p->HasMetadata("HideInInspector");

            bool visible = false;
            if (isPublic)
            {
                visible = !hasHide;
            }
            else // Private/Protected
            {
                visible = hasEditAnywhere;
            }

            if (!visible) continue;

            std::string cat = p->GetMetadata("Category");
            if (!cat.empty())
            {
                categories[cat].push_back(p);
            }
            else
            {
                uncategorized.push_back(p);
            }
        }

        bool any = false;

        // Helper to draw a list of properties
        auto DrawPropertyList = [&](const std::vector<const Bix::Reflection::PropertyInfo*>& list, const std::string& idSuffix)
        {
            if (ImGui::BeginTable(("##PropsTable_" + idSuffix).c_str(), 2, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable))
            {
                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch);
                
                for (auto* p : list)
                {
                    // Filter internal/hidden properties
                    if (p->Name == "lastTexture_" || 
                        p->Name == "hasCustomUV_" || 
                        p->Name == "uvRect_")
                    {
                        continue;
                    }

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    
                    const std::string label = ExposedVariableUtils::MakeDisplayName(p->Name);
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted(label.c_str());
                    
                    // ... (rest of loop)
                    
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s (%s)", p->Name.c_str(), p->TypeName.c_str());

                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushItemWidth(-1);
                    if (DrawReflectedProperty(*p, instance))
                        any = true;
                    ImGui::PopItemWidth();
                }
                ImGui::EndTable();
            }
        };

        // Draw Default Category first
        if (!uncategorized.empty())
        {
            // If we have other categories, show "Default" header, otherwise show nothing (flat)
            if (!categories.empty())
            {
                if (ImGui::CollapsingHeader("Default", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Indent();
                    DrawPropertyList(uncategorized, "Default");
                    ImGui::Unindent();
                }
            }
            else
            {
                 DrawPropertyList(uncategorized, "DefaultFlat");
            }
        }

        // Draw Named Categories
        for (const auto& [catName, catProps] : categories)
        {
            if (ImGui::CollapsingHeader(catName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Indent();
                DrawPropertyList(catProps, catName);
                ImGui::Unindent();
            }
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

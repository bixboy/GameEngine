#include "Gui/Widgets/Controls/PropertyDrawer.h"

#include <unordered_map>
#include <functional>

#include "Gui/Widgets/Controls/DrawVector3Control.h"
#include "Gui/Utils/ExposedVariableUtils.h"
#include "Utils/ReflectionUtils.h"

#include "Ressources/RessourcesClass/Texture.h"
#include "Ressources/RessourcesClass/AudioClip.h"
#include "Ressources/RessourcesClass/SpriteAtlas.h"
#include "Ressources/RessourcesClass/AudioContainer.h"

#include <imgui.h>
#include <filesystem>
#include <cmath>

#include "Gui/Widgets/Controls/DrawBoolControl.h"
#include "Gui/Widgets/Controls/DrawColorControl.h"
#include "Gui/Widgets/Controls/DrawDragControl.h"
#include "Gui/Widgets/Controls/DrawStringControl.h"
#include "Gui/Widgets/Controls/DrawVector2Control.h"
#include "Math/Vector3.h"
#include "Math/Vector2.h"
#include "SDL3/SDL_pixels.h"
#include "Utils/FileIO/FilesUtils.h"


namespace BixEngine::Gui
{
    using namespace Reflection;

    static std::string MakeLabel(const std::string& name)
    {
        return ExposedVariableUtils::MakeDisplayName(name);
    }

    struct InputTextCallback_UserData
    {
        std::string* Str;
    };

    static int InputTextCallback(ImGuiInputTextCallbackData* data)
    {
        auto* user_data = static_cast<InputTextCallback_UserData*>(data->UserData);
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
        {
            std::string* str = user_data->Str;
            IM_ASSERT(data->Buf == str->c_str());
            
            str->resize(data->BufTextLen);
            
            data->Buf = const_cast<char*>(str->c_str());
        }
        
        return 0;
    }
    
    bool PropertyDrawer::DrawPropertyRow(const PropertyInfo& prop, void* instance)
    {
        ImGui::PushID(prop.Name.c_str());
        
        bool isArray = prop.TypeName.find("vector<") != std::string::npos || prop.TypeName.find("TArray<") != std::string::npos;
        
        if (isArray)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            
            bool changed = DispatchWidget(prop, instance);
            ImGui::PopID();
            return changed;
        }

        ImGui::TableNextRow();
        
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(MakeLabel(prop.Name).c_str());
        
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("%s (%s)", prop.Name.c_str(), prop.TypeName.c_str());

        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1);

        bool changed = DispatchWidget(prop, instance);

        ImGui::PopID();
        return changed;
    }

    bool PropertyDrawer::DispatchWidget(const PropertyInfo& prop, void* instance)
    {
        using DrawerFunc = std::function<bool(const PropertyInfo&, void*)>;

        static const std::unordered_map<std::string, DrawerFunc> exactTypeDrawers = 
        {
            // Primitives
            { "bool", DrawBool },
            { "int", DrawInt },
            { "int32_t", DrawInt },
            { "float", DrawFloat },
            { "double", DrawDouble },
            { "String", DrawString },
            { "std::string", DrawString },
            
            // Maths
            { "Math::Vector2", DrawVector2 }, 
            { "Math::Vector2<float>", DrawVector2 },
            { "BixEngine::Math::Vector2", DrawVector2 },
            { "Math::Vector3", DrawVector3 },
            { "Math::Vector3<float>", DrawVector3 },
            { "BixEngine::Math::Vector3", DrawVector3 },
            { "SDL_Color", DrawColor },

            // Ressources Spécifiques
            { "AudioClip", [](const auto& p, void* i) { return DrawResourcePicker<Resources::AudioClip>(p, i, {".mp3", ".wav", ".ogg"}); } },
            { "SpriteAtlas", [](const auto& p, void* i) { return DrawResourcePicker<Resources::SpriteAtlas>(p, i, {".atlas"}); } },
            { "AudioContainer", [](const auto& p, void* i) { return DrawResourcePicker<Resources::AudioContainer>(p, i, {".bixaudio"}); } },
            // Autres ressources simples
        };

        const std::string& type = prop.TypeName;

        if (auto it = exactTypeDrawers.find(type); it != exactTypeDrawers.end())
        {
            return it->second(prop, instance);
        }

        if (type.find("Texture") != std::string::npos)
            return DrawTexture(prop, instance);

        if (type.find("TSubclassOf<") != std::string::npos)
        {
            if (type.find("vector") != std::string::npos || type.find("TArray") != std::string::npos)
                return DrawTSubclassOfArray(prop, instance);
            
            return DrawTSubclassOf(prop, instance);
        }

        if (type.find("vector") != std::string::npos && (type.find("String") != std::string::npos || type.find("string") != std::string::npos))
            return DrawStringArray(prop, instance);

        ImGui::TextDisabled("Unsupported: %s", type.c_str());
        return false;
    }

    // --- Implementations ---

    bool PropertyDrawer::DrawBool(const PropertyInfo& p, void* i)
    {
        return Widgets::DrawBoolControl("##v", p.Get<bool>(i));
    }

    bool PropertyDrawer::DrawInt(const PropertyInfo& p, void* i)
    {
        return Widgets::DrawDragControl("##v", p.Get<int>(i));
    }

    bool PropertyDrawer::DrawFloat(const PropertyInfo& p, void* i)
    {
       return Widgets::DrawDragControl("##v", p.Get<float>(i), 0.1f);
    }

    bool PropertyDrawer::DrawDouble(const PropertyInfo& p, void* i)
    {
        return Widgets::DrawDragControl("##v", p.Get<double>(i), 0.1f);
    }

    bool PropertyDrawer::DrawVector2(const PropertyInfo& p, void* i)
    {
        if (p.TypeName.find("int") != std::string::npos || p.TypeName == "Vec2i")
        {
            auto& vec = p.Get<Math::Vector2<int>>(i);
            return Widgets::DrawVector2Control("##v", &vec.x, 0, 1.0f);
        }
        else
        {
            auto& vec = p.Get<Math::Vector2<float>>(i);
            return Widgets::DrawVector2Control("##v", &vec.x, 0.0f, 0.1f, "%.3f");
        }
    }

    bool PropertyDrawer::DrawVector3(const PropertyInfo& p, void* i)
    {
        if (p.TypeName.find("int") != std::string::npos || p.TypeName == "Vec3i")
        {
            auto& v = p.Get<Math::Vector3<int>>(i);
            return Widgets::DrawVector3Control("##v", &v.x, 0, 1.0f);
        }
        else
        {
            auto& v = p.Get<Math::Vector3<float>>(i);
            return Widgets::DrawVector3Control("##v", &v.x, 0.0f, 0.1f, "%.3f");
        }
    }

    bool PropertyDrawer::DrawColor(const PropertyInfo& p, void* i)
    {
        return Widgets::DrawColorControl("##v", p.Get<SDL_Color>(i));
    }

    bool PropertyDrawer::DrawString(const PropertyInfo& p, void* i)
    {
        std::string* strPtr;
    
        if (p.TypeName == "String")
        {
            strPtr = const_cast<std::string*>(&p.Get<String>(i).Std());
        }
        else
        {
            strPtr = &p.Get<std::string>(i);
        }

        if (!strPtr)
            return false;
    
        ImGui::SetNextItemWidth(-1);
        return Widgets::DrawStringControl("##v", *strPtr);
    }
    
    // --- Implementations Complexes ---

   bool PropertyDrawer::DrawTexture(const PropertyInfo& p, void* instance)
    {
        Resources::Texture*& tex = p.Get<Resources::Texture*>(instance);

        ImGui::BeginGroup();
        
        float previewSize = 64.0f;
        ImVec2 sizeVec(previewSize, previewSize);
        bool openSelector = false;

        if (tex == nullptr)
        {
            if (ImGui::Button("Empty\n(Click)", sizeVec))
                openSelector = true;
        }
        else
        {
            ImTextureID imgID = reinterpret_cast<ImTextureID>(tex->GetNativeHandle());
            if (ImGui::ImageButton("##TextureBtn", imgID, sizeVec, ImVec2(0,0), ImVec2(1,1), ImVec4(0,0,0,1)))
                openSelector = true;

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Size: %fx%f", tex->GetWidth(), tex->GetHeight());
                ImGui::Image(imgID, ImVec2(256, 256));
                ImGui::EndTooltip();
            }
            
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                {
                    const char* path = static_cast<const char*>(payload->Data);
                    auto res = Resources::ResourceManager::Get().Get<Resources::Texture>(path);
                    
                    if (res)
                    {
                        tex = res.get();
                        return true;
                    }
                }
                
                ImGui::EndDragDropTarget();
            }
        }

        ImGui::SameLine();
        ImGui::BeginGroup();
        if (tex)
        {
            ImGui::TextDisabled("%fx%f", tex->GetWidth(), tex->GetHeight());
            if (ImGui::Button("Clear"))
            {
                tex = nullptr;
                
                ImGui::EndGroup();
                ImGui::EndGroup();
                
                return true;
            }
        }
        else
        {
            ImGui::TextDisabled("None");
        }
        
        ImGui::EndGroup();

        if (openSelector)
            ImGui::OpenPopup("TextureSelectorPopup");

        if (ImGui::BeginPopup("TextureSelectorPopup"))
        {
            static char searchBuffer[128] = "";
            ImGui::InputTextWithHint("##Search", "Search...", searchBuffer, sizeof(searchBuffer));
            ImGui::Separator();

            if (ImGui::Selectable("None"))
            {
                tex = nullptr;
                ImGui::CloseCurrentPopup();
            }

            auto foundFiles = BixEngine::Utils::FileUtils::ScanDirectory("", { ".png", ".jpg", ".jpeg", ".tga", ".bmp" });
            for (const auto& path : foundFiles)
            {
                std::string filename = path.filename().string();
                if (searchBuffer[0] != '\0' && filename.find(searchBuffer) == std::string::npos)
                    continue;

                std::string pathStr = path.generic_string();
                bool selected = tex && (std::filesystem::path(tex->GetPath().Std()) == path);

                if (ImGui::Selectable(filename.c_str(), selected))
                {
                    auto sharedTex = Resources::ResourceManager::Get().Get<Resources::Texture>(pathStr);
                    if (sharedTex)
                        tex = sharedTex.get();
                    
                    ImGui::CloseCurrentPopup();
                }
            }
            
            ImGui::EndPopup();
        }

        ImGui::EndGroup();
        return false;
    }

    bool PropertyDrawer::DrawTSubclassOf(const PropertyInfo& p, void* instance)
    {
        auto& pathStr = p.Get<String>(instance);
        
        String currentPath = pathStr;
        std::string display = "None";
        
        if (!currentPath.empty())
            display = std::filesystem::path(currentPath.c_str()).filename().string();

        if (ImGui::Button(display.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        {
            ImGui::OpenPopup("AssetSelectorPopup_Single");
        }

        // Drag Drop
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                const char* draggedPath = static_cast<const char*>(payload->Data);
                pathStr = draggedPath;
                
                ImGui::EndDragDropTarget();
                
                return true;
            }
            ImGui::EndDragDropTarget();
        }

        // Popup
        if (ImGui::BeginPopup("AssetSelectorPopup_Single"))
        {
             static char searchBuffer[128] = "";
             ImGui::InputTextWithHint("##Search", "Search Prefab...", searchBuffer, sizeof(searchBuffer));
             ImGui::Separator();

             if (ImGui::Selectable("None"))
             {
                 pathStr = "";
                 ImGui::CloseCurrentPopup();
             }

             auto foundFiles = BixEngine::Utils::FileUtils::ScanDirectory("", { ".prefab" });
             for (const auto& path : foundFiles)
             {
                 std::string pStr = path.generic_string();
                 std::string filename = path.filename().string();

                 if (searchBuffer[0] != '\0' && filename.find(searchBuffer) == std::string::npos)
                     continue;

                 if (ImGui::Selectable(filename.c_str(), currentPath == pStr.c_str()))
                 {
                     pathStr = pStr.c_str();
                     ImGui::CloseCurrentPopup();
                 }
             }
             ImGui::EndPopup();
        }
        return false;
    }

    // --- Containers ---

    bool PropertyDrawer::DrawStringArray(const PropertyInfo& prop, void* instance)
    {
        auto& list = prop.Get<std::vector<String>>(instance);
        
        bool open = ImGui::TreeNodeEx(prop.Name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
        if (open)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(1);
            if (ImGui::Button("Add Element"))
                list.emplace_back();

            for (size_t i = 0; i < list.size(); ++i)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("  [%zu]", i);

                ImGui::TableSetColumnIndex(1);
                ImGui::PushID(static_cast<int>(i));
                
                if (ImGui::Button("X")) 
                { 
                    list.erase(list.begin() + static_cast<std::ptrdiff_t>(i));
                    ImGui::PopID(); 
                    continue; 
                }
                
                ImGui::SameLine();
                
                char buf[256];
                strncpy_s(buf, list[i].c_str(), 255);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::InputText("##Val", buf, 256))
                {
                    list[i] = buf;
                }

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                    {
                        const char* path = static_cast<const char*>(payload->Data);
                        list[i] = path;
                    }
                    ImGui::EndDragDropTarget();
                }
                
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
        return false;
    }

    bool PropertyDrawer::DrawTSubclassOfArray(const PropertyInfo& prop, void* instance)
    {
        auto& list = prop.Get<std::vector<String>>(instance);

        bool open = ImGui::TreeNodeEx(prop.Name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
        if (open)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(1);
            if (ImGui::Button("Add Prefab")) list.emplace_back();

            for (size_t i = 0; i < list.size(); ++i)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("  [%zu]", i);

                ImGui::TableSetColumnIndex(1);
                ImGui::PushID(static_cast<int>(i));
                
                if (ImGui::Button("X"))
                {
                    list.erase(list.begin() + static_cast<std::ptrdiff_t>(i));
                    ImGui::PopID();
                    continue;
                }
                
                ImGui::SameLine();
                
                String& currentPath = list[i];
                std::string display = currentPath.empty() ? "None" : std::filesystem::path(currentPath.c_str()).filename().string();
                
                ImGui::SetNextItemWidth(-1);
                if (ImGui::Button(display.c_str()))
                    ImGui::OpenPopup("ArrayAssetSelector");

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                    {
                        list[i] = static_cast<const char*>(payload->Data);
                    }
                    
                    ImGui::EndDragDropTarget();
                }

                if (ImGui::BeginPopup("ArrayAssetSelector"))
                {
                    static char sBuf[128] = "";
                    ImGui::InputText("Search", sBuf, 128);
                    
                    if(ImGui::Selectable("None"))
                        list[i] = "";
                    
                    auto files = BixEngine::Utils::FileUtils::ScanDirectory("", {".prefab"});
                    for(const auto& f : files)
                    {
                         std::string fn = f.filename().string();
                         if(sBuf[0] != 0 && fn.find(sBuf) == std::string::npos)
                             continue;
                        
                         if(ImGui::Selectable(fn.c_str()))
                             list[i] = f.generic_string().c_str();
                    }
                    
                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }
            
            ImGui::TreePop();
        }
        
        return false;
    }
}
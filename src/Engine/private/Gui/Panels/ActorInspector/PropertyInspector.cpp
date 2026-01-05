#include "Gui/Panels/ActorInspector/PropertyInspector.h"
#include "Gui/Utils/GuiHelpers.h"

#include <map>
#include <vector>
#include <string>
#include "Gui/Widgets/Controls/PropertyDrawer.h"


namespace BixEngine::Gui::ActorInspector
{
    using namespace Bix::Reflection;

    static bool IsPropertyVisible(const PropertyInfo& p)
    {
        bool isPublic = p.GetMetadata("Access") == "Public";
        bool hasEditAnywhere = p.HasMetadata("EditAnywhere");
        bool hasHide = p.HasMetadata("HideInInspector");

        if (isPublic)
            return !hasHide;
        
        return hasEditAnywhere;
    }

    bool PropertyInspector::DrawClassProperties(const ClassInfo& classInfo, void* instance, bool includeHeader, const char* headerLabel, bool showEmptyMessage)
    {
        // 1. Collecte
        std::vector<const PropertyInfo*> props;
        auto Gather = [&](const ClassInfo& c, auto& refGather) -> void
        {
            if (c.SuperClass)
                refGather(*c.SuperClass, refGather);
            
            for (const auto& p : c.Properties)
                props.push_back(&p);
        };
        Gather(classInfo, Gather);

        // 2. Filtrage & Catégorisation
        std::map<std::string, std::vector<const PropertyInfo*>> categories;
        std::vector<const PropertyInfo*> uncategorized;
        std::vector<std::string> processedNames;

        bool hasAnyVisible = false;

        for (auto* p : props)
        {
            if (!IsPropertyVisible(*p))
                continue;
            
            bool exists = false;
            for(const auto& n : processedNames) if(n == p->Name) { exists = true; break; }
            if(exists)
                continue;
            
            processedNames.push_back(p->Name);

            hasAnyVisible = true;
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

        if (!hasAnyVisible)
        {
            if (showEmptyMessage)
                Utils::DrawEmptyStateMessage("No editable properties.");
            
            return false;
        }

        if (includeHeader && headerLabel)
            Utils::DrawSeparatorText(headerLabel);

        bool changed = false;

        auto DrawGroup = [&](const std::vector<const PropertyInfo*>& list)
        {
            if (list.empty()) return;

            if (ImGui::BeginTable("##PropTable", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerH))
            {
                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                for (const auto* p : list)
                {
                    if (PropertyDrawer::DrawPropertyRow(*p, instance))
                        changed = true;
                }
                ImGui::EndTable();
            }
        };

        // 3. Rendu
        
        if (!uncategorized.empty())
        {
            if (!categories.empty())
            {
                if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Indent(); 
                    DrawGroup(uncategorized); 
                    ImGui::Unindent();
                }
            }
            else
            {
                DrawGroup(uncategorized);
            }
        }

        for (const auto& [catName, catProps] : categories)
        {
            if (ImGui::CollapsingHeader(catName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Indent();
                DrawGroup(catProps);
                ImGui::Unindent();
            }
        }

        return changed;
    }
}

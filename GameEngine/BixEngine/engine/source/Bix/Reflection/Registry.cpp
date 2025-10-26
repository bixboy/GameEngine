#include "Bix/Reflection/Registry.h"

#include "Bix/Reflection/ClassInfo.h"

namespace Bix::Reflection
{
    Registry& Registry::Get()
    {
        static Registry instance;
        return instance;
    }

    void Registry::Register(ClassInfo* classInfo)
    {
        if (!classInfo)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        if (!classInfo->QualifiedName.empty())
        {
            if (byQualifiedName_.find(classInfo->QualifiedName) != byQualifiedName_.end())
            {
                return;
            }
        }
        else if (!classInfo->Name.empty())
        {
            if (byName_.find(classInfo->Name) != byName_.end())
            {
                return;
            }
        }

        classes_.push_back(classInfo);

        if (!classInfo->Name.empty())
        {
            byName_.emplace(classInfo->Name, classInfo);
        }

        if (!classInfo->QualifiedName.empty())
        {
            byQualifiedName_.emplace(classInfo->QualifiedName, classInfo);
        }
    }

    ClassInfo* Registry::Find(std::string_view name)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = byName_.find(std::string{name});
        return it != byName_.end() ? it->second : nullptr;
    }

    ClassInfo* Registry::FindByQualifiedName(std::string_view name)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = byQualifiedName_.find(std::string{name});
        return it != byQualifiedName_.end() ? it->second : nullptr;
    }

    std::vector<ClassInfo*> Registry::GetClasses()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return classes_;
    }
}

#include "Reflection/Registry.h"

#include <iostream>

#include "Reflection/ClassInfo.h"

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

        std::cout << "[REGISTER] " << classInfo->QualifiedName << std::endl;


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

    void Registry::RegisterBaseType(const std::type_info& type, ClassInfo* classInfo)
    {
        if (!classInfo)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        const std::type_index typeKey{type};
        if (baseTypesByType_.find(typeKey) != baseTypesByType_.end())
        {
            return;
        }

        baseTypesByType_.emplace(typeKey, classInfo);

        const std::string& qualified = !classInfo->QualifiedName.empty()
                                            ? classInfo->QualifiedName
                                            : classInfo->Name;

        if (!qualified.empty())
        {
            baseTypesByQualifiedName_.emplace(qualified, classInfo);
        }
    }

    bool Registry::IsBaseType(const std::type_info& type) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return baseTypesByType_.find(std::type_index{type}) != baseTypesByType_.end();
    }

    bool Registry::IsBaseType(std::string_view qualifiedName) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return baseTypesByQualifiedName_.find(std::string{qualifiedName}) != baseTypesByQualifiedName_.end();
    }

    std::vector<ClassInfo*> Registry::GetBaseTypes() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<ClassInfo*> result;
        result.reserve(baseTypesByType_.size());
        for (const auto& [_, info] : baseTypesByType_)
        {
            result.push_back(info);
        }
        return result;
    }
}

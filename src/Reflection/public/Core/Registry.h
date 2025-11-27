#pragma once
#include <mutex>
#include <string>
#include <string_view>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <vector>


namespace Bix::Reflection
{
    struct ClassInfo;

    class Registry
    {
    public:
        static Registry& Get();

        void Register(ClassInfo* classInfo);

        ClassInfo* Find(std::string_view name);
        ClassInfo* FindByQualifiedName(std::string_view name);

        std::vector<ClassInfo*> GetClasses();

        void RegisterBaseType(const std::type_info& type, ClassInfo* classInfo);

        bool IsBaseType(const std::type_info& type) const;
        bool IsBaseType(std::string_view qualifiedName) const;

        std::vector<ClassInfo*> GetBaseTypes() const;

    private:
        Registry() = default;

        mutable std::mutex mutex_;
        std::vector<ClassInfo*> classes_;
        std::unordered_map<std::string, ClassInfo*> byName_;
        std::unordered_map<std::string, ClassInfo*> byQualifiedName_;
        std::unordered_map<std::type_index, ClassInfo*> baseTypesByType_;
        std::unordered_map<std::string, ClassInfo*> baseTypesByQualifiedName_;
    };

    template <typename BaseType>
    void RegisterBaseType(ClassInfo& info)
    {
        Registry::Get().RegisterBaseType(typeid(BaseType), &info);
    }

    template <typename BaseType>
    bool IsBaseTypeRegistered()
    {
        return Registry::Get().IsBaseType(typeid(BaseType));
    }
}

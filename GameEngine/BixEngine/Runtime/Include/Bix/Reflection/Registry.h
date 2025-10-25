#pragma once

#include <mutex>
#include <string>
#include <string_view>
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

    private:
        Registry() = default;

        std::mutex mutex_;
        std::vector<ClassInfo*> classes_;
        std::unordered_map<std::string, ClassInfo*> byName_;
        std::unordered_map<std::string, ClassInfo*> byQualifiedName_;
    };
}

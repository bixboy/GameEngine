#pragma once

#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Bix::Reflection
{
    struct ClassInfo;

    /**
     * @brief Thread-safe registry that stores the metadata for all reflected classes.
     */
    class Registry
    {
    public:
        /**
         * @brief Provides access to the singleton registry instance.
         */
        static Registry& Get();

        /**
         * @brief Registers a class metadata entry if it has not been inserted yet.
         */
        void Register(ClassInfo* classInfo);

        /**
         * @brief Finds a class by its simple name.
         */
        ClassInfo* Find(std::string_view name);

        /**
         * @brief Finds a class by its fully qualified name.
         */
        ClassInfo* FindByQualifiedName(std::string_view name);

        /**
         * @brief Returns every registered class.
         */
        std::vector<ClassInfo*> GetClasses();

    private:
        Registry() = default;

        std::mutex mutex_;
        std::vector<ClassInfo*> classes_;
        std::unordered_map<std::string, ClassInfo*> byName_;
        std::unordered_map<std::string, ClassInfo*> byQualifiedName_;
    };
}

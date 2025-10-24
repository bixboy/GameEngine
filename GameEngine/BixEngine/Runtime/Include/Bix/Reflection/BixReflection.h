#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <new>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace BixEngine::Game
{
    class Actor;
    class Component;
}

namespace Bix::Reflection
{
    struct ClassInfo;

    struct PropertyInfo
    {
        using Getter = void* (*)(void*);
        using ConstGetter = const void* (*)(const void*);

        std::string Name;
        std::string TypeName;
        std::size_t Offset = 0;
        bool HasOffset = false;
        std::size_t Size = 0;
        const ClassInfo* Owner = nullptr;
        Getter Access = nullptr;
        ConstGetter ConstAccess = nullptr;

        [[nodiscard]] bool IsValid() const noexcept
        {
            return Access != nullptr && ConstAccess != nullptr;
        }

        void* GetRaw(void* instance) const
        {
            return Access ? Access(instance) : nullptr;
        }

        const void* GetRaw(const void* instance) const
        {
            return ConstAccess ? ConstAccess(instance) : nullptr;
        }

        template<typename T>
        T& Get(void* instance) const
        {
            return *static_cast<T*>(Access(instance));
        }

        template<typename T>
        const T& Get(const void* instance) const
        {
            return *static_cast<const T*>(ConstAccess(instance));
        }
    };

    struct ClassInfo
    {
        std::string Name;
        std::string QualifiedName;
        std::size_t Size = 0;
        ClassInfo* SuperClass = nullptr;
        std::vector<PropertyInfo> Properties;
        bool IsAbstract = false;

        using Constructor = void* (*)(void* context);
        Constructor ConstructorFn = nullptr;

        [[nodiscard]] const PropertyInfo* FindProperty(std::string_view name) const
        {
            for (const auto& property : Properties)
            {
                if (property.Name == name)
                {
                    return &property;
                }
            }

            if (SuperClass)
            {
                return SuperClass->FindProperty(name);
            }

            return nullptr;
        }

        PropertyInfo* FindProperty(std::string_view name)
        {
            for (auto& property : Properties)
            {
                if (property.Name == name)
                {
                    return &property;
                }
            }

            if (SuperClass)
            {
                return SuperClass->FindProperty(name);
            }

            return nullptr;
        }

        [[nodiscard]] bool CanConstruct() const noexcept
        {
            return ConstructorFn != nullptr;
        }

        void* Construct(void* context = nullptr) const
        {
            return ConstructorFn ? ConstructorFn(context) : nullptr;
        }

        template<typename T>
        T* ConstructTyped(void* context = nullptr) const
        {
            return static_cast<T*>(Construct(context));
        }
    };

    class Registry
    {
    public:
        static Registry& Get()
        {
            static Registry instance;
            return instance;
        }

        void Register(ClassInfo* classInfo)
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

        ClassInfo* Find(std::string_view name)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = byName_.find(std::string{name});
            if (it != byName_.end())
            {
                return it->second;
            }
            return nullptr;
        }

        ClassInfo* FindByQualifiedName(std::string_view name)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = byQualifiedName_.find(std::string{name});
            if (it != byQualifiedName_.end())
            {
                return it->second;
            }
            return nullptr;
        }

        std::vector<ClassInfo*> GetClasses()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return classes_;
        }

    private:
        Registry() = default;

        std::mutex mutex_;
        std::vector<ClassInfo*> classes_;
        std::unordered_map<std::string, ClassInfo*> byName_;
        std::unordered_map<std::string, ClassInfo*> byQualifiedName_;
    };

    inline ClassInfo* FindClass(std::string_view name)
    {
        return Registry::Get().Find(name);
    }

    inline ClassInfo* FindClassByQualifiedName(std::string_view name)
    {
        return Registry::Get().FindByQualifiedName(name);
    }

    inline std::vector<ClassInfo*> GetAllClasses()
    {
        return Registry::Get().GetClasses();
    }

    namespace detail
    {
        template<typename ClassType>
        struct ClassRegistrationInvoker
        {
            ClassRegistrationInvoker()
            {
                (void)ClassType::StaticClass();
            }
        };

        template<typename ClassType, typename Populator>
        ClassInfo& RegisterClass(const char* name,
                                 const char* qualifiedName,
                                 ClassInfo* superClass,
                                 Populator&& populator)
        {
            static std::once_flag onceFlag;
            static ClassInfo classInfo;

            std::call_once(onceFlag, [&]() {
                classInfo.Name = name ? name : "";
                classInfo.QualifiedName = qualifiedName ? qualifiedName : classInfo.Name;
                classInfo.Size = sizeof(ClassType);
                classInfo.SuperClass = superClass;
                classInfo.Properties.clear();
                classInfo.ConstructorFn = nullptr;

                populator(classInfo);

                for (auto& property : classInfo.Properties)
                {
                    property.Owner = &classInfo;
                }

                classInfo.IsAbstract = std::is_abstract_v<ClassType>;

                Registry::Get().Register(&classInfo);
            });

            return classInfo;
        }

        template<typename ClassType, typename PropertyType>
        PropertyInfo& RegisterProperty(ClassInfo& classInfo,
                                       const char* name,
                                       PropertyType ClassType::* member,
                                       const char* displayTypeName)
        {
            PropertyInfo& info = classInfo.Properties.emplace_back();
            if (name)
            {
                info.Name = name;
            }
            if (displayTypeName)
            {
                info.TypeName = displayTypeName;
            }
            else
            {
                info.TypeName = typeid(PropertyType).name();
            }

            info.Size = sizeof(PropertyType);
            info.Access = [member](void* instance) -> void* {
                auto* object = static_cast<ClassType*>(instance);
                return &(object->*member);
            };
            info.ConstAccess = [member](const void* instance) -> const void* {
                auto* object = static_cast<const ClassType*>(instance);
                return &(object->*member);
            };

            if constexpr (std::is_standard_layout_v<ClassType> && std::is_default_constructible_v<ClassType>)
            {
                std::aligned_storage_t<sizeof(ClassType), alignof(ClassType)> storage{};
                ClassType* instance = new (&storage) ClassType();
                auto* base = reinterpret_cast<std::byte*>(static_cast<void*>(instance));
                auto* memberPtr = reinterpret_cast<std::byte*>(&(instance->*member));
                info.Offset = static_cast<std::size_t>(memberPtr - base);
                info.HasOffset = true;
                instance->~ClassType();
            }
            else
            {
                info.Offset = 0;
                info.HasOffset = false;
            }

            return info;
        }
    }
}

#define BCLASS()
#define BPROPERTY()

#ifndef GENERATED_BODY
#    define GENERATED_BODY(...) static_assert(false, "Missing generated header for this class. Did you run BixHeaderTool?")
#endif


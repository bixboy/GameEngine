#pragma once

#include "Reflection/ClassInfo.h"
#include "Reflection/PropertyInfo.h"
#include "Reflection/Registry.h"

#include <cstddef>
#include <mutex>
#include <new>
#include <type_traits>
#include <typeinfo>

namespace Bix::Reflection::detail
{

    template<typename ClassType, typename Populator>
        inline ClassInfo& RegisterReflectedClass(const char* name,
                                                 const char* qualifiedName,
                                                 ClassInfo* superClass,
                                                 Populator&& populator)
    {
        return RegisterClass<ClassType, Populator>(
            name,
            qualifiedName,
            superClass,
            std::forward<Populator>(populator)
        );
    }
    
    /**
     * @brief Forces the registration of a class at static initialization time.
     */
    template<typename ClassType>
    struct ClassRegistrationInvoker
    {
        ClassRegistrationInvoker()
        {
            (void)ClassType::StaticClass();
        }
    };

    /**
     * @brief Registers the metadata of a class and returns the resulting descriptor.
     */
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

            classInfo.IsAbstract = std::is_abstract_v<ClassType>;
            Registry::Get().Register(&classInfo);
        });

        return classInfo;
    }

    /**
     * @brief Registers a property of a class inside the reflection metadata.
     */
    template<typename ClassType, typename PropertyType>
    PropertyInfo& RegisterProperty(ClassInfo& classInfo,
                                   const char* name,
                                   PropertyType ClassType::* member,
                                   const char* displayTypeName)
    {
        PropertyInfo& info = classInfo.Properties.emplace_back();

        if (name)
            info.Name = name;

        info.TypeName = displayTypeName ? displayTypeName : typeid(PropertyType).name();
        info.Size = sizeof(PropertyType);
        info.Owner = &classInfo;

        info.Access = [member](void* instance) -> void*
        {
            auto* object = static_cast<ClassType*>(instance);
            return &(object->*member);
        };

        info.ConstAccess = [member](const void* instance) -> const void*
        {
            auto* object = static_cast<const ClassType*>(instance);
            return &(object->*member);
        };

        if constexpr (std::is_standard_layout_v<ClassType> && std::is_default_constructible_v<ClassType>)
        {
            using Storage = std::aligned_storage_t<sizeof(ClassType), alignof(ClassType)>;
            Storage storage{};
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

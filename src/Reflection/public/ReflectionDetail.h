#pragma once
#include "ClassInfo.h"
#include "PropertyInfo.h"
#include "Registry.h"
#include <cstddef>
#include <mutex>
#include <type_traits>
#include <typeinfo>
#include <utility>


namespace Bix::Reflection::detail
{
    template <typename Base, typename Derived, typename = void>
    struct SafeIsBaseOf : std::false_type
    {
    };

    template <typename Base, typename Derived>
    struct SafeIsBaseOf<Base, Derived, std::void_t<decltype(sizeof(Base))>> : std::bool_constant<std::is_base_of_v<Base, Derived>>
    {
    };

    template <typename Base, typename Derived>
    inline constexpr bool SafeIsBaseOf_v = SafeIsBaseOf<Base, Derived>::value;

    inline constexpr std::size_t kInvalidRootIndex = static_cast<std::size_t>(-1);

    template <std::size_t Index, typename DeclaringTypeT>
    struct RootTag : std::integral_constant<std::size_t, Index>
    {
        using DeclaringType = DeclaringTypeT;
    };

    template <typename T, typename = void>
    struct RootTagTraits
    {
        static constexpr std::size_t Index = kInvalidRootIndex;
        using DeclaringType = void;
        static constexpr bool HasTag = false;
    };

    template <typename T>
    struct RootTagTraits<T, std::void_t<typename T::__BixReflection_RootTag>>
    {
        using TagType = T::__BixReflection_RootTag;

        static constexpr std::size_t Index = TagType::value;

        using DeclaringType = TagType::DeclaringType;

        static constexpr bool HasTag = true;
    };

    template <typename T, std::size_t Index>
    inline constexpr bool HasRootTag_v = RootTagTraits<std::remove_cv_t<T>>::HasTag && RootTagTraits<std::remove_cv_t<
        T>>::Index == Index;

    template <typename T>
    inline constexpr bool HasAnyRootTag_v = RootTagTraits<std::remove_cv_t<T>>::HasTag;

    template <typename T, typename = void>
    struct HasActorTag : std::false_type
    {
    };

    template <typename T>
    struct HasActorTag<T, std::void_t<typename T::__BixReflection_ActorTag>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool HasActorTag_v = HasActorTag<std::remove_cv_t<T>>::value;

    template <typename T>
    constexpr bool IsEngineBaseType()
    {
        using Traits = RootTagTraits<std::remove_cv_t<T>>;
        if constexpr (!Traits::HasTag)
        {
            return false;
        }
        else
        {
            return std::is_same_v<std::remove_cv_t<T>, typename Traits::DeclaringType>;
        }
    }

    template <typename T>
    constexpr bool IsRegisteredBaseType()
    {
        return IsEngineBaseType<T>();
    }

    template <typename ClassType, typename Populator>
    ClassInfo& RegisterReflectedClass(const char* name, const char* qualifiedName, ClassInfo* superClass,
                                      Populator&& populator)
    {
        return RegisterClass<ClassType, Populator>(
            name,
            qualifiedName,
            superClass,
            std::forward<Populator>(populator)
        );
    }

    template <typename ClassType>
    struct ClassRegistrationInvoker
    {
        ClassRegistrationInvoker()
        {
            (void)ClassType::StaticClass();
        }
    };


    template <typename ClassType, typename Populator>
    ClassInfo& RegisterClass(const char* name, const char* qualifiedName, ClassInfo* superClass, Populator&& populator)
    {
        static std::once_flag onceFlag;
        static ClassInfo classInfo;

        std::call_once(onceFlag, [&]()
        {
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


    template <typename ClassType, typename PropertyType>
    PropertyInfo& RegisterProperty(ClassInfo& classInfo, const char* name, PropertyType ClassType::* member,
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
            ClassType* instance = new(&storage) ClassType();

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

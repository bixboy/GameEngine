#pragma once


#include "Core/BixReflection.h"

#ifdef GENERATED_BODY
#undef GENERATED_BODY
#endif

#define GENERATED_BODY() \
public: \
    using SuperClassType = ::BixEngine::Game::Object; \
    using Super = SuperClassType; \
    using __BixReflection_ActorTag = void; \
    static ::Bix::Reflection::ClassInfo& StaticClass() \
    { \
        using ThisClass = ::BixEngine::Game::Actor; \
        ::Bix::Reflection::ClassInfo* superClass = nullptr; \
        superClass = &::BixEngine::Game::Object::StaticClass(); \
        auto& classInfo = ::Bix::Reflection::detail::RegisterReflectedClass<ThisClass>( \
            "Actor", \
            "BixEngine::Game::Actor", \
            superClass, \
            [](::Bix::Reflection::ClassInfo& info) \
            { \
                (void)info; \
                if constexpr (std::is_abstract_v<ThisClass>) \
                { info.ConstructorFn = nullptr; } \
                else if constexpr (::Bix::Reflection::detail::SafeIsBaseOf_v<::BixEngine::Game::Component, ThisClass>) \
                { \
                    info.ConstructorFn = +[](void* context) -> void* \
                    { \
                        auto* owner = static_cast<::BixEngine::Game::Actor*>(context); \
                        if (!owner) return nullptr; \
                        if constexpr (std::is_constructible_v<ThisClass, ::BixEngine::Game::Actor*>) \
                            return static_cast<void*>(new ThisClass(owner)); \
                        else if constexpr (std::is_default_constructible_v<ThisClass>) \
                            return static_cast<void*>(new ThisClass()); \
                        else return nullptr; \
                    }; \
                } \
                else if constexpr (::Bix::Reflection::detail::SafeIsBaseOf_v<::BixEngine::Game::Actor, ThisClass>) \
                { \
                    if constexpr (std::is_default_constructible_v<ThisClass>) \
                        info.ConstructorFn = +[](void*) -> void* { return static_cast<void*>(new ThisClass()); }; \
                    else info.ConstructorFn = nullptr; \
                } \
                else if constexpr (std::is_default_constructible_v<ThisClass>) \
                { \
                    info.ConstructorFn = +[](void*) -> void* { return static_cast<void*>(new ThisClass()); }; \
                } \
                else \
                { \
                    info.ConstructorFn = nullptr; \
                } \
            }); \
        return classInfo; \
    } \
    virtual ::Bix::Reflection::ClassInfo& GetClass() const { return StaticClass(); } \
private: \
    static inline ::Bix::Reflection::detail::ClassRegistrationInvoker<::BixEngine::Game::Actor> __BixReflection_AutoRegister{};

#pragma once


#include "Core/BixReflection.h"

#ifdef GENERATED_BODY
#undef GENERATED_BODY
#endif

#define GENERATED_BODY() \
public: \
    using SuperClassType = void; \
    using __BixReflection_RootTag = ::Bix::Reflection::detail::RootTag<0, ::BixEngine::Game::Object>; \
    static ::Bix::Reflection::ClassInfo& StaticClass() \
    { \
        using ThisClass = ::BixEngine::Game::Object; \
        ::Bix::Reflection::ClassInfo* superClass = nullptr; \
        auto& classInfo = ::Bix::Reflection::detail::RegisterReflectedClass<ThisClass>( \
            "Object", \
            "BixEngine::Game::Object", \
            superClass, \
            [](::Bix::Reflection::ClassInfo& info) \
            { \
                (void)info; \
                if constexpr (std::is_abstract_v<ThisClass>) \
                { info.ConstructorFn = nullptr; } \
                else if constexpr (std::is_default_constructible_v<ThisClass>) \
                { info.ConstructorFn = +[](void*) -> void* { return static_cast<void*>(new ThisClass()); }; } \
                else \
                { info.ConstructorFn = nullptr; } \
                ::Bix::Reflection::RegisterBaseType<ThisClass>(info); \
            }); \
        return classInfo; \
    } \
    virtual ::Bix::Reflection::ClassInfo& GetClass() const { return StaticClass(); } \
private: \
    static inline ::Bix::Reflection::detail::ClassRegistrationInvoker<::BixEngine::Game::Object> __BixReflection_AutoRegister{};

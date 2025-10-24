#pragma once
// Auto-generated stub for reflection. In real workflows this file is produced by BixHeaderTool.

#include "Bix/Reflection/BixReflection.h"

#ifdef GENERATED_BODY
#    undef GENERATED_BODY
#endif

#define GENERATED_BODY()                                                            \
public:                                                                             \
    static ::Bix::Reflection::ClassInfo& StaticClass()                              \
    {                                                                               \
        using ThisClass = ::BixEngine::Game::Object;                                \
        ::Bix::Reflection::ClassInfo* superClass = nullptr;                         \
        auto& classInfo = ::Bix::Reflection::detail::RegisterClass<ThisClass>(      \
            "Object",                                                              \
            "::BixEngine::Game::Object",                                           \
            superClass,                                                             \
            [](::Bix::Reflection::ClassInfo& info)                                  \
            {                                                                       \
                info.ConstructorFn = +[](void*) -> void*                            \
                {                                                                   \
                    return static_cast<void*>(new ThisClass());                     \
                };                                                                  \
            });                                                                     \
        return classInfo;                                                           \
    }                                                                               \
    virtual ::Bix::Reflection::ClassInfo& GetClass() const                          \
    {                                                                               \
        return StaticClass();                                                       \
    }                                                                               \
                                                                                    \
private:                                                                            \
    static inline ::Bix::Reflection::detail::ClassRegistrationInvoker<              \
        ::BixEngine::Game::Object>                                                  \
        __BixReflection_AutoRegister{}

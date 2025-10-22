#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "Bix/Core/String.h"

namespace BixEngine::Game
{
    class Actor;
}

namespace BixEngine::Game::Scripting
{
    enum class ScriptKind
    {
        Unknown,
        Actor,
        Component,
        Widget,
        Scene,
        Ui,
        Service
    };

    struct ScriptInstantiationParams
    {
        Game::Actor* owner{nullptr};
        void* userData{nullptr};
    };

    class ScriptBase;

    using ScriptFactory = std::function<std::unique_ptr<ScriptBase>(const ScriptInstantiationParams&)>;

    struct ScriptClass
    {
        String name;
        String displayName;
        String moduleName;
        String nativeName;
        ScriptKind kind{ScriptKind::Unknown};
        std::size_t size{0};
        bool isAbstract{false};
        bool canInstantiate{false};
        ScriptFactory factory;
        ScriptClass* superClass{nullptr};
        std::vector<ScriptClass*> derivedClasses;

        [[nodiscard]] std::unique_ptr<ScriptBase> Instantiate(const ScriptInstantiationParams& params = {}) const;

        template<typename T>
        [[nodiscard]] std::unique_ptr<T> InstantiateAs(const ScriptInstantiationParams& params = {}) const
        {
            auto baseInstance = Instantiate(params);
            if (!baseInstance)
                return nullptr;

            auto* typed = dynamic_cast<T*>(baseInstance.release());
            if (!typed)
                return nullptr;

            return std::unique_ptr<T>(typed);
        }
    };

    struct ScriptClassDescriptor
    {
        String name;
        String displayName;
        String moduleName;
        String nativeName;
        ScriptKind kind{ScriptKind::Unknown};
        std::size_t size{0};
        bool isAbstract{false};
        ScriptFactory factory;
    };

    struct ScriptRegistrationDescriptor
    {
        const char* name{nullptr};
        const char* displayName{nullptr};
        const char* moduleName{nullptr};
        const char* nativeName{nullptr};
        ScriptKind kind{ScriptKind::Unknown};
        ScriptFactory factory{};
        bool isAbstract{false};
    };

    class ScriptRegistry
    {
    public:
        ScriptRegistry();

        ScriptRegistry(const ScriptRegistry&) = delete;
        ScriptRegistry& operator=(const ScriptRegistry&) = delete;

        static ScriptRegistry& Get();

        ScriptClass& RegisterClass(ScriptClassDescriptor descriptor, ScriptClass* superClass);

        [[nodiscard]] ScriptClass* FindClass(std::string_view name) noexcept;
        [[nodiscard]] const ScriptClass* FindClass(std::string_view name) const noexcept;

        [[nodiscard]] ScriptClass& RequireClass(std::string_view name);
        [[nodiscard]] const ScriptClass& RequireClass(std::string_view name) const;

        [[nodiscard]] std::vector<const ScriptClass*> GetClasses() const;
        [[nodiscard]] std::vector<const ScriptClass*> GetClasses(ScriptKind kind) const;

        void ForEach(const std::function<void(const ScriptClass&)>& visitor) const;

        [[nodiscard]] std::unique_ptr<ScriptBase> Instantiate(std::string_view name, const ScriptInstantiationParams& params = {}) const;

        template<typename T>
        [[nodiscard]] std::unique_ptr<T> InstantiateAs(std::string_view name, const ScriptInstantiationParams& params = {}) const
        {
            auto baseInstance = Instantiate(name, params);
            if (!baseInstance)
                return nullptr;

            auto* typed = dynamic_cast<T*>(baseInstance.release());
            if (!typed)
                return nullptr;

            return std::unique_ptr<T>(typed);
        }

    private:
        struct Entry
        {
            ScriptClass info;
        };

        [[nodiscard]] static std::string NormalizeName(std::string_view name);

        mutable std::mutex mutex_;
        std::unordered_map<std::string, std::unique_ptr<Entry>> classes_;
    };

    class ScriptBase
    {
    public:
        virtual ~ScriptBase() = default;

        [[nodiscard]] virtual const ScriptClass& GetScriptClass() const = 0;

        static ScriptClass& StaticScriptClass();
    };

    template<typename T>
    [[nodiscard]] inline ScriptFactory MakeDefaultFactory()
    {
        if constexpr (std::is_abstract_v<T>)
        {
            return {};
        }
        else if constexpr (std::is_constructible_v<T, Game::Actor*>)
        {
            return [](const ScriptInstantiationParams& params) -> std::unique_ptr<ScriptBase>
            {
                if (!params.owner)
                    return nullptr;
                return std::unique_ptr<ScriptBase>(new T(params.owner));
            };
        }
        else if constexpr (std::is_default_constructible_v<T>)
        {
            return [](const ScriptInstantiationParams&) -> std::unique_ptr<ScriptBase>
            {
                return std::unique_ptr<ScriptBase>(new T());
            };
        }
        else
        {
            return {};
        }
    }

    template<typename TClass, typename TSuper>
    class ScriptRegistration
    {
    public:
        explicit ScriptRegistration(ScriptRegistrationDescriptor descriptor)
        {
            if (!descriptor.name)
                descriptor.name = typeid(TClass).name();
            if (!descriptor.displayName)
                descriptor.displayName = descriptor.name;
            if (!descriptor.moduleName)
                descriptor.moduleName = "Runtime";
            if (!descriptor.nativeName)
                descriptor.nativeName = typeid(TClass).name();

            const bool isAbstract = descriptor.isAbstract || std::is_abstract_v<TClass>;

            ScriptClassDescriptor classDescriptor
            {
                .name = descriptor.name,
                .displayName = descriptor.displayName,
                .moduleName = descriptor.moduleName,
                .nativeName = descriptor.nativeName,
                .kind = descriptor.kind,
                .size = sizeof(TClass),
                .isAbstract = isAbstract,
                .factory = descriptor.factory ? descriptor.factory : MakeDefaultFactory<TClass>(),
            };

            ScriptClass* super = nullptr;
            if constexpr (std::is_base_of_v<ScriptBase, TSuper>)
            {
                super = &TSuper::StaticScriptClass();
            }

            scriptClass_ = &ScriptRegistry::Get().RegisterClass(std::move(classDescriptor), super);
        }

        [[nodiscard]] ScriptClass& GetScriptClass() const noexcept { return *scriptClass_; }

    private:
        ScriptClass* scriptClass_{nullptr};
    };
}

#ifndef BIX_SCRIPT_DEFAULT_MODULE
#define BIX_SCRIPT_DEFAULT_MODULE "Runtime"
#endif

#define BIX_SCRIPT_STRINGIZE_IMPL(x) #x
#define BIX_SCRIPT_STRINGIZE(x) BIX_SCRIPT_STRINGIZE_IMPL(x)

#ifdef BIX_SCRIPT_ENABLE_AUTOGEN
#define BIX_SCRIPT_INCLUDE_GENERATED_HEADER(ClassName) \
    BIX_SCRIPT_INCLUDE_GENERATED_HEADER_IMPL(#ClassName ".generated.h")
#define BIX_SCRIPT_INCLUDE_GENERATED_HEADER_IMPL(HeaderName) \
    BIX_SCRIPT_INCLUDE_GENERATED_HEADER_INVOKE(HeaderName)
#define BIX_SCRIPT_INCLUDE_GENERATED_HEADER_INVOKE(HeaderName) \
    #include HeaderName
#else
#define BIX_SCRIPT_INCLUDE_GENERATED_HEADER(ClassName)
#endif

#define BIX_DETAIL_GENERATED_BODY_IMPL(_1, N, ...) BIX_DETAIL_GENERATED_BODY_##N(_1)
#define BIX_DETAIL_GENERATED_BODY_0(_) BIX_SCRIPT_GENERATED_BODY_NOARGS()
#define BIX_DETAIL_GENERATED_BODY_1(ClassName) BIX_SCRIPT_INCLUDE_GENERATED_HEADER(ClassName)

#define BIX_GENERATED_BODY(...) BIX_DETAIL_GENERATED_BODY_IMPL(__VA_ARGS__, 1, 0)

#define BIX_SCRIPT_GENERATED_BODY_NOARGS() \
    BIX_SCRIPT_GENERATED_BODY_FROM_PENDING()

#define BIX_SCRIPT_GENERATED_BODY_FROM_PENDING() \
    BIX_SCRIPT_GENERATED_BODY_FROM_PENDING_IMPL()

#define BIX_SCRIPT_GENERATED_BODY_FROM_PENDING_IMPL() \
    #if !defined(BIX_SCRIPT_PENDING_CLASS) || !defined(BIX_SCRIPT_PENDING_SUPER) \
        #error "BCLASS(ClassType, ScriptSuperType[, SaveSuperType]) must be declared before BIX_GENERATED_BODY()" \
    #endif \
    BIX_SCRIPT_INCLUDE_GENERATED_HEADER(BIX_SCRIPT_PENDING_CLASS) \
    BIX_DECLARE_SCRIPT_CLASS(BIX_SCRIPT_PENDING_CLASS, BIX_SCRIPT_PENDING_SUPER); \
    BIX_SCRIPT_APPLY_SAVE_METADATA() \
    using ThisScriptClass = BIX_SCRIPT_PENDING_CLASS; \
    using Super = BIX_SCRIPT_PENDING_SUPER; \
    BIX_SCRIPT_CLEAR_PENDING()

#define BIX_SCRIPT_APPLY_SAVE_METADATA() \
    #ifdef BIX_SAVE_PENDING_SUPER \
        BIX_CLASS(BIX_SCRIPT_PENDING_CLASS, BIX_SAVE_PENDING_SUPER); \
        using SaveSuper = BIX_SAVE_PENDING_SUPER; \
    #endif

#define BIX_SCRIPT_CLEAR_PENDING() \
    #undef BIX_SCRIPT_PENDING_CLASS \
    #undef BIX_SCRIPT_PENDING_SUPER \
    #ifdef BIX_SAVE_PENDING_SUPER \
        #undef BIX_SAVE_PENDING_SUPER \
    #endif

#define BIX_DETAIL_BCLASS_SELECT(_1, _2, _3, NAME, ...) NAME

#define BCLASS(...) \
    BIX_DETAIL_BCLASS_SELECT(__VA_ARGS__, BIX_DETAIL_BCLASS_WITH_SAVE, BIX_DETAIL_BCLASS_WITHOUT_SAVE, /*unused*/)(__VA_ARGS__)

#define BSTRUCT(...) BCLASS(__VA_ARGS__)
#define BENUM(...)

#define BIX_DETAIL_BCLASS_WITH_SAVE(ClassType, ScriptSuperType, SaveSuperType) \
    class ClassType; \
    #undef BIX_SCRIPT_PENDING_CLASS \
    #undef BIX_SCRIPT_PENDING_SUPER \
    #undef BIX_SAVE_PENDING_SUPER \
    #define BIX_SCRIPT_PENDING_CLASS ClassType \
    #define BIX_SCRIPT_PENDING_SUPER ScriptSuperType \
    #define BIX_SAVE_PENDING_SUPER SaveSuperType

#define BIX_DETAIL_BCLASS_WITHOUT_SAVE(ClassType, ScriptSuperType) \
    class ClassType; \
    #undef BIX_SCRIPT_PENDING_CLASS \
    #undef BIX_SCRIPT_PENDING_SUPER \
    #undef BIX_SAVE_PENDING_SUPER \
    #define BIX_SCRIPT_PENDING_CLASS ClassType \
    #define BIX_SCRIPT_PENDING_SUPER ScriptSuperType

#define BIX_DECLARE_SCRIPT_CLASS(ClassType, BaseType) \
public: \
    using ThisScriptClass = ClassType; \
    using SuperScriptClass = BaseType; \
    static ::BixEngine::Game::Scripting::ScriptClass& StaticScriptClass(); \
    [[nodiscard]] const ::BixEngine::Game::Scripting::ScriptClass& GetScriptClass() const override; \
private: \
    static ::BixEngine::Game::Scripting::ScriptRegistration<ClassType, BaseType> s_scriptRegistration_; \
public:

#define BIX_DEFINE_SCRIPT_CLASS(ClassType, Descriptor) \
    ::BixEngine::Game::Scripting::ScriptRegistration<ClassType, ClassType::SuperScriptClass> \
        ClassType::s_scriptRegistration_{Descriptor}; \
    ::BixEngine::Game::Scripting::ScriptClass& ClassType::StaticScriptClass() \
    { \
        return ClassType::s_scriptRegistration_.GetScriptClass(); \
    } \
    const ::BixEngine::Game::Scripting::ScriptClass& ClassType::GetScriptClass() const \
    { \
        return ClassType::StaticScriptClass(); \
    }


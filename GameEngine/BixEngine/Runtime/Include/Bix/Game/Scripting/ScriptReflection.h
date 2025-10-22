#pragma once

#include <filesystem>
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

        void EnableAutoSaveManifest(const std::filesystem::path& manifestPath);
        void DisableAutoSaveManifest();

    private:
        struct Entry
        {
            ScriptClass info;
        };

        [[nodiscard]] static std::string NormalizeName(std::string_view name);
        void SaveManifestLocked_() const;

        mutable std::mutex mutex_;
        std::unordered_map<std::string, std::unique_ptr<Entry>> classes_;
        std::filesystem::path manifestPath_{};
        bool autoSaveManifest_{false};
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
#define BIX_GENERATED_BODY(ClassName) BIX_SCRIPT_INCLUDE_GENERATED_HEADER(ClassName)
#else
#define BIX_GENERATED_BODY(ClassName)
#endif

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


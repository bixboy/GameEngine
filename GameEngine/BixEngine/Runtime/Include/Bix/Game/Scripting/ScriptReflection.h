#pragma once

#include <cctype>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Bix/Core/String.h"

namespace BixEngine::Game
{
    class Actor;
}

namespace BixEngine::Game::Scripting
{
    using ScriptMetadataEntry = std::pair<const char*, const char*>;
    using ScriptMetadataMap = std::unordered_map<std::string, String>;

    namespace Detail
    {
        [[nodiscard]] std::vector<String> ParseKeywordList(const char* keywords);
        [[nodiscard]] ScriptMetadataMap BuildMetadataMap(const ScriptMetadataEntry* entries, std::size_t count);
    }

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
        String category;
        String tooltip;
        std::vector<String> keywords;
        ScriptMetadataMap metadata;
        bool editorOnly{false};
        bool deprecated{false};
        bool hideInEditor{false};

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

        [[nodiscard]] const String& GetCategory() const noexcept { return category; }
        [[nodiscard]] const String& GetTooltip() const noexcept { return tooltip; }
        [[nodiscard]] const std::vector<String>& GetKeywords() const noexcept { return keywords; }
        [[nodiscard]] const ScriptMetadataMap& GetMetadata() const noexcept { return metadata; }
        [[nodiscard]] bool IsEditorOnly() const noexcept { return editorOnly; }
        [[nodiscard]] bool IsDeprecated() const noexcept { return deprecated; }
        [[nodiscard]] bool IsHiddenInEditor() const noexcept { return hideInEditor; }
        [[nodiscard]] bool IsEditorVisible() const noexcept { return !hideInEditor; }

        [[nodiscard]] const String* FindMetadata(std::string_view key) const
        {
            if (key.empty())
                return nullptr;

            auto it = metadata.find(std::string(key));
            if (it != metadata.end())
                return &it->second;
            return nullptr;
        }

        [[nodiscard]] bool MatchesKeyword(std::string_view keyword, bool caseSensitive = false) const
        {
            if (keyword.empty())
                return true;

            for (const auto& entry : keywords)
            {
                if (caseSensitive)
                {
                    if (entry.View() == keyword)
                        return true;
                }
                else if (entry.EqualsIgnoreCase(keyword))
                {
                    return true;
                }
            }

            return false;
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
        String category;
        String tooltip;
        std::vector<String> keywords;
        ScriptMetadataMap metadata;
        bool editorOnly{false};
        bool deprecated{false};
        bool hideInEditor{false};
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
        const char* category{nullptr};
        const char* tooltip{nullptr};
        const char* keywords{nullptr};
        const ScriptMetadataEntry* metadata{nullptr};
        std::size_t metadataCount{0};
        bool editorOnly{false};
        bool deprecated{false};
        bool hideInEditor{false};
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
        [[nodiscard]] std::vector<const ScriptClass*> GetClassesForEditor(bool includeEditorOnly = false) const;
        [[nodiscard]] std::vector<const ScriptClass*> GetClassesForEditor(ScriptKind kind, bool includeEditorOnly = false) const;
        [[nodiscard]] std::vector<const ScriptClass*> GetClassesInModule(std::string_view moduleName, bool includeEditorOnly = false, bool includeHidden = false) const;
        [[nodiscard]] std::vector<String> GetModuleNames() const;
        [[nodiscard]] std::vector<const ScriptClass*> GetDerivedClasses(std::string_view name, bool recursive = false) const;

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
        [[nodiscard]] static std::string NormalizeModuleName(std::string_view moduleName);
        void UpdateModuleCache(const std::string& previousModule, ScriptClass& scriptClass);

        mutable std::mutex mutex_;
        std::unordered_map<std::string, std::unique_ptr<Entry>> classes_;
        std::unordered_map<std::string, std::vector<ScriptClass*>> modules_;
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
                .category = descriptor.category ? descriptor.category : "",
                .tooltip = descriptor.tooltip ? descriptor.tooltip : "",
                .keywords = Detail::ParseKeywordList(descriptor.keywords),
                .metadata = Detail::BuildMetadataMap(descriptor.metadata, descriptor.metadataCount),
                .editorOnly = descriptor.editorOnly,
                .deprecated = descriptor.deprecated,
                .hideInEditor = descriptor.hideInEditor,
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

namespace BixEngine::Game::Scripting::Detail
{
    [[nodiscard]] inline std::vector<String> ParseKeywordList(const char* keywords)
    {
        std::vector<String> result;
        if (!keywords || keywords[0] == '\0')
            return result;

        std::string_view view{keywords};
        std::size_t start = 0;
        while (start < view.size())
        {
            std::size_t end = start;
            while (end < view.size() && view[end] != ',' && view[end] != ';' && view[end] != '|')
                ++end;

            std::string_view token = view.substr(start, end - start);
            while (!token.empty() && std::isspace(static_cast<unsigned char>(token.front())) != 0)
                token.remove_prefix(1);
            while (!token.empty() && std::isspace(static_cast<unsigned char>(token.back())) != 0)
                token.remove_suffix(1);

            if (!token.empty())
                result.emplace_back(token);

            start = (end == view.size()) ? view.size() : end + 1;
        }

        return result;
    }

    [[nodiscard]] inline ScriptMetadataMap BuildMetadataMap(const ScriptMetadataEntry* entries, std::size_t count)
    {
        ScriptMetadataMap metadata;
        if (!entries || count == 0)
            return metadata;

        metadata.reserve(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            const auto& entry = entries[index];
            if (!entry.first)
                continue;

            const char* value = entry.second ? entry.second : "";
            metadata.emplace(entry.first, String(value));
        }

        return metadata;
    }
}

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


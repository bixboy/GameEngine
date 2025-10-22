#include "Bix/Game/Scripting/ScriptReflection.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace BixEngine::Game::Scripting;

namespace
{
    constexpr std::string_view kScriptBaseName{"ScriptBase"};

    void SortByDisplayName(std::vector<const ScriptClass*>& classes)
    {
        std::sort(classes.begin(), classes.end(), [](const ScriptClass* lhs, const ScriptClass* rhs)
        {
            const std::string_view lhsName = lhs ? lhs->displayName.View() : std::string_view{};
            const std::string_view rhsName = rhs ? rhs->displayName.View() : std::string_view{};
            if (lhsName == rhsName)
                return (lhs ? lhs->name.View() : std::string_view{}) < (rhs ? rhs->name.View() : std::string_view{});
            return lhsName < rhsName;
        });
    }
}

ScriptRegistry::ScriptRegistry() = default;

ScriptRegistry& ScriptRegistry::Get()
{
    static ScriptRegistry instance;
    return instance;
}

std::string ScriptRegistry::NormalizeName(std::string_view name)
{
    return std::string(name);
}

std::string ScriptRegistry::NormalizeModuleName(std::string_view moduleName)
{
    if (moduleName.empty())
        return std::string(BIX_SCRIPT_DEFAULT_MODULE);

    return std::string(moduleName);
}

ScriptClass& ScriptRegistry::RegisterClass(ScriptClassDescriptor descriptor, ScriptClass* superClass)
{
    std::scoped_lock lock(mutex_);

    auto normalized = NormalizeName(descriptor.name.Std());
    auto it = classes_.find(normalized);

    std::string previousModule{};
    ScriptClass* previousSuper{nullptr};

    if (it == classes_.end())
    {
        auto entry = std::make_unique<Entry>();
        entry->info.name = std::move(descriptor.name);
        entry->info.displayName = descriptor.displayName.IsEmpty() ? entry->info.name : std::move(descriptor.displayName);
        entry->info.moduleName = descriptor.moduleName.IsEmpty() ? String(BIX_SCRIPT_DEFAULT_MODULE) : std::move(descriptor.moduleName);
        entry->info.nativeName = descriptor.nativeName.IsEmpty() ? entry->info.name : std::move(descriptor.nativeName);
        entry->info.kind = descriptor.kind;
        entry->info.size = descriptor.size;
        entry->info.isAbstract = descriptor.isAbstract;
        entry->info.factory = std::move(descriptor.factory);
        entry->info.canInstantiate = entry->info.factory && !entry->info.isAbstract;
        entry->info.superClass = superClass;
        entry->info.category = std::move(descriptor.category);
        entry->info.tooltip = std::move(descriptor.tooltip);
        entry->info.keywords = std::move(descriptor.keywords);
        entry->info.metadata = std::move(descriptor.metadata);
        entry->info.editorOnly = descriptor.editorOnly;
        entry->info.deprecated = descriptor.deprecated;
        entry->info.hideInEditor = descriptor.hideInEditor;

        auto [insertedIt, inserted] = classes_.emplace(normalized, std::move(entry));
        it = insertedIt;
        (void)inserted;
    }
    else
    {
        auto& info = it->second->info;
        previousModule = NormalizeModuleName(info.moduleName.Std());
        previousSuper = info.superClass;
        info.displayName = descriptor.displayName.IsEmpty() ? info.name : std::move(descriptor.displayName);
        if (!descriptor.moduleName.IsEmpty())
            info.moduleName = std::move(descriptor.moduleName);
        if (!descriptor.nativeName.IsEmpty())
            info.nativeName = std::move(descriptor.nativeName);
        info.kind = descriptor.kind;
        info.size = descriptor.size;
        info.isAbstract = descriptor.isAbstract;
        info.factory = std::move(descriptor.factory);
        info.canInstantiate = info.factory && !info.isAbstract;
        info.superClass = superClass;
        info.category = std::move(descriptor.category);
        info.tooltip = std::move(descriptor.tooltip);
        info.keywords = std::move(descriptor.keywords);
        info.metadata = std::move(descriptor.metadata);
        info.editorOnly = descriptor.editorOnly;
        info.deprecated = descriptor.deprecated;
        info.hideInEditor = descriptor.hideInEditor;
    }

    ScriptClass& scriptClass = it->second->info;

    if (previousSuper && previousSuper != scriptClass.superClass)
    {
        auto& siblings = previousSuper->derivedClasses;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), &scriptClass), siblings.end());
    }

    if (superClass)
    {
        auto& children = superClass->derivedClasses;
        if (std::find(children.begin(), children.end(), &scriptClass) == children.end())
            children.push_back(&scriptClass);
    }

    UpdateModuleCache(previousModule, scriptClass);

    return scriptClass;
}

ScriptClass* ScriptRegistry::FindClass(std::string_view name) noexcept
{
    std::scoped_lock lock(mutex_);
    auto it = classes_.find(NormalizeName(name));
    if (it != classes_.end())
        return &it->second->info;
    return nullptr;
}

const ScriptClass* ScriptRegistry::FindClass(std::string_view name) const noexcept
{
    std::scoped_lock lock(mutex_);
    auto it = classes_.find(NormalizeName(name));
    if (it != classes_.end())
        return &it->second->info;
    return nullptr;
}

ScriptClass& ScriptRegistry::RequireClass(std::string_view name)
{
    if (auto* scriptClass = FindClass(name))
        return *scriptClass;
    throw std::runtime_error("Script class not found: " + std::string(name));
}

const ScriptClass& ScriptRegistry::RequireClass(std::string_view name) const
{
    if (auto* scriptClass = FindClass(name))
        return *scriptClass;
    throw std::runtime_error("Script class not found: " + std::string(name));
}

std::vector<const ScriptClass*> ScriptRegistry::GetClasses() const
{
    std::scoped_lock lock(mutex_);
    std::vector<const ScriptClass*> result;
    result.reserve(classes_.size());
    for (const auto& [_, entry] : classes_)
        result.push_back(&entry->info);
    return result;
}

std::vector<const ScriptClass*> ScriptRegistry::GetClasses(ScriptKind kind) const
{
    std::scoped_lock lock(mutex_);
    std::vector<const ScriptClass*> result;
    for (const auto& [_, entry] : classes_)
    {
        if (entry->info.kind == kind)
            result.push_back(&entry->info);
    }
    return result;
}

std::vector<const ScriptClass*> ScriptRegistry::GetClassesForEditor(bool includeEditorOnly) const
{
    std::scoped_lock lock(mutex_);
    std::vector<const ScriptClass*> result;
    result.reserve(classes_.size());
    for (const auto& [_, entry] : classes_)
    {
        const ScriptClass& scriptClass = entry->info;
        if (scriptClass.hideInEditor)
            continue;
        if (!includeEditorOnly && scriptClass.editorOnly)
            continue;
        result.push_back(&scriptClass);
    }

    SortByDisplayName(result);
    return result;
}

std::vector<const ScriptClass*> ScriptRegistry::GetClassesForEditor(ScriptKind kind, bool includeEditorOnly) const
{
    std::scoped_lock lock(mutex_);
    std::vector<const ScriptClass*> result;
    for (const auto& [_, entry] : classes_)
    {
        const ScriptClass& scriptClass = entry->info;
        if (scriptClass.kind != kind)
            continue;
        if (scriptClass.hideInEditor)
            continue;
        if (!includeEditorOnly && scriptClass.editorOnly)
            continue;
        result.push_back(&scriptClass);
    }

    SortByDisplayName(result);
    return result;
}

std::vector<const ScriptClass*> ScriptRegistry::GetClassesInModule(std::string_view moduleName, bool includeEditorOnly, bool includeHidden) const
{
    std::scoped_lock lock(mutex_);
    std::vector<const ScriptClass*> result;
    const auto normalized = NormalizeModuleName(moduleName);
    auto it = modules_.find(normalized);
    if (it == modules_.end())
        return result;

    const auto& entries = it->second;
    result.reserve(entries.size());
    for (const ScriptClass* scriptClass : entries)
    {
        if (!scriptClass)
            continue;
        if (!includeHidden && scriptClass->hideInEditor)
            continue;
        if (!includeEditorOnly && scriptClass->editorOnly)
            continue;
        result.push_back(scriptClass);
    }

    SortByDisplayName(result);
    return result;
}

std::vector<String> ScriptRegistry::GetModuleNames() const
{
    std::scoped_lock lock(mutex_);
    std::vector<String> result;
    result.reserve(modules_.size());
    for (const auto& [moduleName, classes] : modules_)
    {
        if (classes.empty())
            continue;
        result.emplace_back(moduleName);
    }

    std::sort(result.begin(), result.end(), [](const String& lhs, const String& rhs)
    {
        return lhs.View() < rhs.View();
    });

    return result;
}

std::vector<const ScriptClass*> ScriptRegistry::GetDerivedClasses(std::string_view name, bool recursive) const
{
    std::scoped_lock lock(mutex_);
    std::vector<const ScriptClass*> result;
    auto it = classes_.find(NormalizeName(name));
    if (it == classes_.end())
        return result;

    const ScriptClass& scriptClass = it->second->info;
    if (!recursive)
    {
        result.reserve(scriptClass.derivedClasses.size());
        for (auto* derived : scriptClass.derivedClasses)
        {
            if (derived)
                result.push_back(derived);
        }
    }
    else
    {
        std::vector<const ScriptClass*> stack;
        for (auto* derived : scriptClass.derivedClasses)
        {
            if (!derived)
                continue;
            result.push_back(derived);
            stack.push_back(derived);
        }

        for (std::size_t index = 0; index < stack.size(); ++index)
        {
            const ScriptClass* current = stack[index];
            for (auto* child : current->derivedClasses)
            {
                if (!child)
                    continue;
                result.push_back(child);
                stack.push_back(child);
            }
        }
    }

    SortByDisplayName(result);
    return result;
}

void ScriptRegistry::ForEach(const std::function<void(const ScriptClass&)>& visitor) const
{
    std::scoped_lock lock(mutex_);
    for (const auto& [_, entry] : classes_)
        visitor(entry->info);
}

std::unique_ptr<ScriptBase> ScriptRegistry::Instantiate(std::string_view name, const ScriptInstantiationParams& params) const
{
    const ScriptClass* scriptClass = FindClass(name);
    if (!scriptClass || !scriptClass->canInstantiate)
        return nullptr;

    return scriptClass->factory ? scriptClass->factory(params) : nullptr;
}

ScriptClass& ScriptBase::StaticScriptClass()
{
    static ScriptClass& scriptClass = []() -> ScriptClass&
    {
        ScriptClassDescriptor descriptor
        {
            .name = String(kScriptBaseName.data()),
            .displayName = String(kScriptBaseName.data()),
            .moduleName = String("Core"),
            .nativeName = String(typeid(ScriptBase).name()),
            .kind = ScriptKind::Unknown,
            .size = sizeof(ScriptBase),
            .isAbstract = true,
            .factory = {},
            .category = String("Core"),
            .tooltip = String("Root script base class."),
            .metadata = {},
            .editorOnly = true,
            .deprecated = false,
            .hideInEditor = true,
        };

        return ScriptRegistry::Get().RegisterClass(std::move(descriptor), nullptr);
    }();

    return scriptClass;
}

std::unique_ptr<ScriptBase> ScriptClass::Instantiate(const ScriptInstantiationParams& params) const
{
    if (!factory || isAbstract)
        return nullptr;

    return factory(params);
}

void ScriptRegistry::UpdateModuleCache(const std::string& previousModule, ScriptClass& scriptClass)
{
    const std::string moduleKey = NormalizeModuleName(scriptClass.moduleName.Std());

    if (!previousModule.empty() && previousModule != moduleKey)
    {
        auto prevIt = modules_.find(previousModule);
        if (prevIt != modules_.end())
        {
            auto& list = prevIt->second;
            list.erase(std::remove(list.begin(), list.end(), &scriptClass), list.end());
            if (list.empty())
                modules_.erase(prevIt);
        }
    }

    auto& moduleEntries = modules_[moduleKey];
    if (std::find(moduleEntries.begin(), moduleEntries.end(), &scriptClass) == moduleEntries.end())
        moduleEntries.push_back(&scriptClass);
}


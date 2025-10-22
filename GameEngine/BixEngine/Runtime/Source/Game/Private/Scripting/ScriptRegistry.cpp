#include "Bix/Game/Scripting/ScriptReflection.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

using namespace BixEngine::Game::Scripting;

namespace
{
    constexpr std::string_view kScriptBaseName{"ScriptBase"};
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

ScriptClass& ScriptRegistry::RegisterClass(ScriptClassDescriptor descriptor, ScriptClass* superClass)
{
    std::scoped_lock lock(mutex_);

    auto normalized = NormalizeName(descriptor.name.Std());
    auto it = classes_.find(normalized);

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

        auto [insertedIt, inserted] = classes_.emplace(normalized, std::move(entry));
        it = insertedIt;
        (void)inserted;
    }
    else
    {
        auto& info = it->second->info;
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
    }

    ScriptClass& scriptClass = it->second->info;

    if (superClass)
    {
        auto& children = superClass->derivedClasses;
        if (std::find(children.begin(), children.end(), &scriptClass) == children.end())
            children.push_back(&scriptClass);
    }

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
            .factory = {}
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


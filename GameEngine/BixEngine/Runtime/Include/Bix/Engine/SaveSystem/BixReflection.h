#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Bix/Core/String.h"

namespace BixEngine::Engine::SaveSystem
{
    class BixArchiveWriter;
    class BixArchiveReader;

    class BixObject;

    template<typename TClass, typename TSuper>
    class ClassRegistrar;

    template<typename TClass>
    struct PropertyRegistry;

    /**
     * Represents a globally unique identifier used by the save system to
     * uniquely identify every reflected object.
     */
    struct BixGuid
    {
        std::array<std::uint8_t, 16> data{};

        [[nodiscard]] bool IsValid() const noexcept;
        [[nodiscard]] String ToString() const;

        static BixGuid NewGuid();
        static BixGuid FromString(std::string_view text);

        friend bool operator==(const BixGuid& lhs, const BixGuid& rhs) noexcept
        {
            return lhs.data == rhs.data;
        }

        friend bool operator!=(const BixGuid& lhs, const BixGuid& rhs) noexcept
        {
            return !(lhs == rhs);
        }
    };

    struct BixGuidHasher
    {
        std::size_t operator()(const BixGuid& guid) const noexcept;
    };

    /**
     * Describes a reflected property that can be serialized by the save
     * system. The property stores both read/write callbacks which operate on
     * the owning object instance.
     */
    struct BixProperty
    {
        using WriteFunction = void(*)(const BixObject&, BixArchiveWriter&);
        using ReadFunction  = void(*)(BixObject&, BixArchiveReader&);

        String name;
        WriteFunction write{nullptr};
        ReadFunction read{nullptr};
    };

    /**
     * Metadata describing a reflected class.
     */
    class BixClass
    {
    public:
        using Factory = std::function<std::unique_ptr<BixObject>()>;

        BixClass(String name, std::string nativeName, std::size_t size, Factory factory, const BixClass* superClass);

        [[nodiscard]] const String& GetName() const noexcept { return name_; }
        [[nodiscard]] const std::string& GetNativeName() const noexcept { return nativeName_; }
        [[nodiscard]] std::size_t GetSize() const noexcept { return size_; }
        [[nodiscard]] const BixClass* GetSuperClass() const noexcept { return superClass_; }
        [[nodiscard]] bool IsAbstract() const noexcept { return factory_ == nullptr; }

        std::unique_ptr<BixObject> CreateInstance() const;

        void AddProperty(BixProperty property);
        [[nodiscard]] const std::vector<BixProperty>& GetProperties() const noexcept { return properties_; }

    private:
        String name_;
        std::string nativeName_;
        std::size_t size_{};
        Factory factory_;
        const BixClass* superClass_{nullptr};
        std::vector<BixProperty> properties_;
    };

    /**
     * Base type for every reflected object. Provides GUID and outer handling
     * which is used to maintain ownership hierarchies.
     */
    class BixObject
    {
    public:
        using ThisClass = BixObject;
        using SuperClass = void;

        BixObject();
        virtual ~BixObject() = default;

        [[nodiscard]] const BixGuid& GetGuid() const noexcept { return guid_; }
        void SetGuid(const BixGuid& guid) noexcept { guid_ = guid; }

        [[nodiscard]] BixObject* GetOuter() noexcept { return outer_; }
        [[nodiscard]] const BixObject* GetOuter() const noexcept { return outer_; }
        void SetOuter(BixObject* outer) noexcept { outer_ = outer; }

        virtual const BixClass& GetClass() const noexcept = 0;
        static BixClass& StaticClass();

        virtual void OnPostDeserialize() {}

    protected:
        static void RegisterProperties(BixClass&) {}

    private:
        static ClassRegistrar<BixObject, void> s_bixClassRegistrar_;

        BixGuid guid_{};
        BixObject* outer_{nullptr};
    };

    /**
     * Global registry storing every reflected class definition.
     */
    class ClassRegistry
    {
    public:
        static ClassRegistry& Get();

        BixClass& RegisterClass(String name,
                                std::string nativeName,
                                std::size_t size,
                                BixClass::Factory factory,
                                const BixClass* superClass);

        [[nodiscard]] const BixClass* FindClass(std::string_view name) const noexcept;
        [[nodiscard]] const BixClass* FindClassByNative(std::string_view name) const noexcept;
        std::unique_ptr<BixObject> CreateInstance(std::string_view name) const;
        void ForEachClass(const std::function<void(const BixClass&)>& visitor) const;

    private:
        std::unordered_map<std::string, std::unique_ptr<BixClass>> classesByName_;
        std::unordered_map<std::string, BixClass*> classesByNativeName_;
        mutable std::mutex mutex_;
    };

    template<typename TClass>
    struct PropertyRegistry
    {
        using Initializer = void(*)(BixClass&);

        static void Register(Initializer initializer)
        {
            GetInitializers().push_back(initializer);
        }

        static void Apply(BixClass& cls)
        {
            for (Initializer initializer : GetInitializers())
            {
                initializer(cls);
            }
        }

    private:
        static std::vector<Initializer>& GetInitializers()
        {
            static std::vector<Initializer> initializers;
            return initializers;
        }
    };

    template<typename TClass, typename TSuper>
    class ClassRegistrar
    {
    public:
        explicit ClassRegistrar(const char* name)
        {
            auto factory = MakeFactory_();
            class_ = &ClassRegistry::Get().RegisterClass(String(name), typeid(TClass).name(), sizeof(TClass), std::move(factory), &TSuper::StaticClass());
            TClass::RegisterProperties(*class_);
            PropertyRegistry<TClass>::Apply(*class_);
        }

        [[nodiscard]] BixClass& GetClass() const noexcept { return *class_; }

    private:
        static BixClass::Factory MakeFactory_()
        {
            if constexpr (std::is_abstract_v<TClass>)
            {
                return nullptr;
            }
            else
            {
                return []() -> std::unique_ptr<BixObject>
                {
                    return std::make_unique<TClass>();
                };
            }
        }

        BixClass* class_{};
    };

    template<typename TClass>
    class ClassRegistrar<TClass, void>
    {
    public:
        explicit ClassRegistrar(const char* name)
        {
            auto factory = MakeFactory_();
            class_ = &ClassRegistry::Get().RegisterClass(String(name), typeid(TClass).name(), sizeof(TClass), std::move(factory), nullptr);
            TClass::RegisterProperties(*class_);
            PropertyRegistry<TClass>::Apply(*class_);
        }

        [[nodiscard]] BixClass& GetClass() const noexcept { return *class_; }

    private:
        static BixClass::Factory MakeFactory_()
        {
            if constexpr (std::is_abstract_v<TClass>)
            {
                return nullptr;
            }
            else
            {
                return []() -> std::unique_ptr<BixObject>
                {
                    return std::make_unique<TClass>();
                };
            }
        }

        BixClass* class_{};
    };

    template<typename T>
    struct AlwaysFalse : std::false_type {};

    template<typename T>
    struct PropertyAdapter;

    template<typename TObject, typename TValue>
    class PropertyRegistration
    {
    public:
        PropertyRegistration(const char* name, TValue TObject::*member)
        {
            PropertyRegistry<TObject>::Register([name, member](BixClass& cls)
            {
                BixProperty property;
                property.name = name;
                property.write = [member](const BixObject& object, BixArchiveWriter& writer)
                {
                    const auto& typed = static_cast<const TObject&>(object);
                    PropertyAdapter<TValue>::Serialize(object, typed.*member, writer);
                };
                property.read = [member](BixObject& object, BixArchiveReader& reader)
                {
                    auto& typed = static_cast<TObject&>(object);
                    PropertyAdapter<TValue>::Deserialize(object, typed.*member, reader);
                };
                cls.AddProperty(std::move(property));
            });
        }
    };

    // Utility macro helpers -------------------------------------------------

    #define BIX_CONCAT_IMPL(a, b) a##b
    #define BIX_CONCAT(a, b) BIX_CONCAT_IMPL(a, b)

    #define BIX_ROOT_CLASS(ClassType) \
    public: \
        using ThisClass = ClassType; \
        using SuperClass = void; \
        static ::BixEngine::Engine::SaveSystem::BixClass& StaticClass(); \
        const ::BixEngine::Engine::SaveSystem::BixClass& GetClass() const noexcept override; \
    private: \
        static ::BixEngine::Engine::SaveSystem::ClassRegistrar<ClassType, void> s_bixClassRegistrar_; \
    public:

    #define BIX_CLASS(ClassType, SuperType) \
    public: \
        using ThisClass = ClassType; \
        using SuperClass = SuperType; \
        static ::BixEngine::Engine::SaveSystem::BixClass& StaticClass(); \
        const ::BixEngine::Engine::SaveSystem::BixClass& GetClass() const noexcept override; \
    private: \
        static ::BixEngine::Engine::SaveSystem::ClassRegistrar<ClassType, SuperType> s_bixClassRegistrar_; \
    public:

    #define BIX_IMPLEMENT_CLASS(ClassType) \
        ::BixEngine::Engine::SaveSystem::ClassRegistrar<ClassType, ClassType::SuperClass> \
            ClassType::s_bixClassRegistrar_{#ClassType}; \
        ::BixEngine::Engine::SaveSystem::BixClass& ClassType::StaticClass() \
        { \
            return ClassType::s_bixClassRegistrar_.GetClass(); \
        } \
        const ::BixEngine::Engine::SaveSystem::BixClass& ClassType::GetClass() const noexcept \
        { \
            return ClassType::StaticClass(); \
        }

    #define BIX_PROPERTY(Type, Name, ...) \
        Type Name __VA_ARGS__; \
        static inline ::BixEngine::Engine::SaveSystem::PropertyRegistration<ThisClass, Type> \
            BIX_CONCAT(s_bixPropertyReg_, __LINE__){#Name, &ThisClass::Name}

} // namespace BixEngine::Engine::SaveSystem


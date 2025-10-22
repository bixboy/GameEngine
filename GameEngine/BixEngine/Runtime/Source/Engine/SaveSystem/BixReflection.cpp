#include "Bix/Engine/SaveSystem/BixReflection.h"

#include <algorithm>
#include <array>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

namespace BixEngine::Engine::SaveSystem
{
    namespace
    {
        constexpr char kGuidHexDigits[] = "0123456789abcdef";
    }

    bool BixGuid::IsValid() const noexcept
    {
        return std::any_of(data.begin(), data.end(), [](std::uint8_t value) { return value != 0; });
    }

    String BixGuid::ToString() const
    {
        std::array<char, 36> buffer{};
        std::size_t index = 0;
        for (std::size_t i = 0; i < data.size(); ++i)
        {
            const std::uint8_t value = data[i];
            buffer[index++] = kGuidHexDigits[(value >> 4) & 0x0F];
            buffer[index++] = kGuidHexDigits[value & 0x0F];

            if (i == 3 || i == 5 || i == 7 || i == 9)
                buffer[index++] = '-';
        }

        return String(std::string_view(buffer.data(), buffer.size()));
    }

    BixGuid BixGuid::NewGuid()
    {
        BixGuid guid;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<std::uint32_t> dist(0, 255);

        for (auto& value : guid.data)
        {
            value = static_cast<std::uint8_t>(dist(gen));
        }

        guid.data[6] = static_cast<std::uint8_t>((guid.data[6] & 0x0F) | 0x40);
        guid.data[8] = static_cast<std::uint8_t>((guid.data[8] & 0x3F) | 0x80);
        return guid;
    }

    BixGuid BixGuid::FromString(std::string_view text)
    {
        BixGuid guid;
        if (text.size() != 36)
            throw std::invalid_argument("GUID string must contain 36 characters.");

        std::size_t dataIndex = 0;
        for (std::size_t i = 0; i < text.size(); )
        {
            if (text[i] == '-')
            {
                ++i;
                continue;
            }

            if (i + 1 >= text.size())
                throw std::invalid_argument("Invalid GUID string length.");

            const auto hexToByte = [](char high, char low) -> std::uint8_t
            {
                const auto decode = [](char ch) -> int
                {
                    if (ch >= '0' && ch <= '9')
                        return ch - '0';
                    if (ch >= 'a' && ch <= 'f')
                        return 10 + (ch - 'a');
                    if (ch >= 'A' && ch <= 'F')
                        return 10 + (ch - 'A');
                    throw std::invalid_argument("Invalid GUID digit.");
                };

                return static_cast<std::uint8_t>((decode(high) << 4) | decode(low));
            };

            guid.data[dataIndex++] = hexToByte(text[i], text[i + 1]);
            i += 2;
        }

        return guid;
    }

    std::size_t BixGuidHasher::operator()(const BixGuid& guid) const noexcept
    {
        std::size_t hash = 0;
        for (std::uint8_t value : guid.data)
        {
            hash = (hash * 131) ^ value;
        }
        return hash;
    }

    BixClass::BixClass(String name, std::string nativeName, std::size_t size, Factory factory, const BixClass* superClass)
        : name_(std::move(name)), nativeName_(std::move(nativeName)), size_(size), factory_(std::move(factory)), superClass_(superClass)
    {
    }

    std::unique_ptr<BixObject> BixClass::CreateInstance() const
    {
        if (!factory_)
            return nullptr;
        return factory_();
    }

    void BixClass::AddProperty(BixProperty property)
    {
        properties_.push_back(std::move(property));
    }

    BixObject::BixObject()
    {
        guid_ = BixGuid::NewGuid();
    }

    BixClass& BixObject::StaticClass()
    {
        return s_bixClassRegistrar_.GetClass();
    }

    ClassRegistrar<BixObject, void> BixObject::s_bixClassRegistrar_{"BixObject"};

    ClassRegistry& ClassRegistry::Get()
    {
        static ClassRegistry registry;
        return registry;
    }

    BixClass& ClassRegistry::RegisterClass(String name,
                                           std::string nativeName,
                                           std::size_t size,
                                           BixClass::Factory factory,
                                           const BixClass* superClass)
    {
        std::scoped_lock lock(mutex_);
        const std::string nameKey = name.Std();
        if (auto it = classesByName_.find(nameKey); it != classesByName_.end())
        {
            return *it->second;
        }

        if (auto it = classesByNativeName_.find(nativeName); it != classesByNativeName_.end())
        {
            return *it->second;
        }

        auto classPtr = std::make_unique<BixClass>(std::move(name), std::move(nativeName), size, std::move(factory), superClass);
        BixClass& reference = *classPtr;
        classesByNativeName_.emplace(reference.GetNativeName(), &reference);
        classesByName_.emplace(nameKey, std::move(classPtr));
        return reference;
    }

    const BixClass* ClassRegistry::FindClass(std::string_view name) const noexcept
    {
        std::scoped_lock lock(mutex_);
        const auto it = classesByName_.find(std::string(name));
        if (it != classesByName_.end())
            return it->second.get();

        const auto nativeIt = classesByNativeName_.find(std::string(name));
        if (nativeIt != classesByNativeName_.end())
            return nativeIt->second;
        return nullptr;
    }

    const BixClass* ClassRegistry::FindClassByNative(std::string_view name) const noexcept
    {
        std::scoped_lock lock(mutex_);
        const auto it = classesByNativeName_.find(std::string(name));
        if (it == classesByNativeName_.end())
            return nullptr;
        return it->second;
    }

    std::unique_ptr<BixObject> ClassRegistry::CreateInstance(std::string_view name) const
    {
        const BixClass* cls = FindClass(name);
        if (!cls)
            return nullptr;
        return cls->CreateInstance();
    }

    void ClassRegistry::ForEachClass(const std::function<void(const BixClass&)>& visitor) const
    {
        std::scoped_lock lock(mutex_);
        for (const auto& [_, classPtr] : classesByName_)
        {
            if (classPtr)
                visitor(*classPtr);
        }
    }
}


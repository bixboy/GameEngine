#include "Bix/Game/Object.h"


namespace BixEngine::Game
{
    using BixGuid = Engine::SaveSystem::BixGuid;

    BIX_IMPLEMENT_CLASS(Object);

    Object::Object() : Object("Object") {}

    Object::Object(String name) : name_(std::move(name)) {}

    Object::Object(String name, const Math::Transform& transform) : name_(std::move(name)), transform_(transform) {}

    String Object::GetTypeName() const noexcept
    {
        return "Object";
    }

    void Object::SetUUID(const String& uuid)
    {
        SetGuid(BixGuid::FromString(uuid.View()));
    }

    void Object::SetName(String name)
    {
        name_ = std::move(name);
    }

    void Object::RegisterProperties(::BixEngine::Engine::SaveSystem::BixClass& cls)
    {
        using ::BixEngine::Engine::SaveSystem::RegisterProperty;

        RegisterProperty<Object>(cls, "name", &Object::name_);
        RegisterProperty<Object>(cls, "transform", &Object::transform_);
    }
}


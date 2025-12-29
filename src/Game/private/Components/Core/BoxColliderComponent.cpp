#include "Components/Core/BoxColliderComponent.h"
#include "Framework/Actor.h"
#include "Framework/Scene.h"
#include "Debug/Logger.h"
#include "Framework/PhysicsConstants.h"
#include <imgui.h>

namespace BixEngine::Game
{
    
    
    

    BoxColliderComponent::BoxColliderComponent() = default;

    BoxColliderComponent::BoxColliderComponent(Actor* owner) : Super(owner)
    {
    }

    BoxColliderComponent::~BoxColliderComponent()
    {
        DestroyPhysicsState();
    }

    void BoxColliderComponent::BeginPlay()
    {
        Super::BeginPlay();
        CreatePhysicsState();
    }

    void BoxColliderComponent::Update(float deltaTime)
    {
        Super::Update(deltaTime);

        if (!b2Body_IsValid(bodyId_)) return;

        
        if (simulatePhysics_ && owner_)
        {
            b2Vec2 pos = b2Body_GetPosition(bodyId_);
            b2Rot rot = b2Body_GetRotation(bodyId_);
            float angleRad = b2Rot_GetAngle(rot);

            Math::Transform& transform = owner_->GetTransformRef();
            transform.position.x = pos.x * Physics::PPM;
            transform.position.y = pos.y * Physics::PPM;
            transform.rotation.yaw = Math::Rad2Deg(angleRad);
            
            
            ApplyAerodynamics();
            ClampTerminalVelocity();
        }
    }

    void BoxColliderComponent::DrawInspectorUI()
    {
        
    }

    
    
    

    void BoxColliderComponent::ApplyAerodynamics()
    {
        
        if (airResistance_ > 0.001f)
        {
            b2Vec2 v = b2Body_GetLinearVelocity(bodyId_);
            b2Vec2 dragForce = { -v.x * airResistance_, -v.y * airResistance_ };
            b2Body_ApplyForceToCenter(bodyId_, dragForce, true);
        }
    }

    void BoxColliderComponent::ClampTerminalVelocity()
    {
        
        b2Vec2 v = b2Body_GetLinearVelocity(bodyId_);
        float maxMeters = maxFallSpeed_ / Physics::PPM;

        if (v.y > maxMeters)
        {
            v.y = maxMeters;
            b2Body_SetLinearVelocity(bodyId_, v);
        }
    }

    void BoxColliderComponent::CreatePhysicsState()
    {
        DestroyPhysicsState();
        if (!owner_) return;

        Scene* scene = owner_->GetOwningScene();
        if (!scene) return;

        b2WorldId worldId = scene->GetPhysicsWorld();
        if (!b2World_IsValid(worldId)) return;

        
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = simulatePhysics_ ? b2_dynamicBody : b2_staticBody;
        bodyDef.fixedRotation = fixedRotation_;
        bodyDef.gravityScale = gravityScale_;
        bodyDef.linearDamping = 0.0f; 

        
        float worldYaw = 0.0f;
        const Math::Transform* current = &owner_->GetTransformRef();
        while (current)
        {
            worldYaw += current->rotation.yaw;
            current = current->parent;
        }

        
        Math::Vector3 worldPos = owner_->GetTransformRef().GetWorldPosition();

        bodyDef.position = { worldPos.x / Physics::PPM, worldPos.y / Physics::PPM };
        bodyDef.rotation = b2MakeRot(Math::Deg2Rad(worldYaw));

        bodyId_ = b2CreateBody(worldId, &bodyDef);
        
        if (b2Body_IsValid(bodyId_))
        {
            b2Body_SetUserData(bodyId_, this);
            UpdateShape();
        }
    }

    void BoxColliderComponent::DestroyPhysicsState()
    {
        if (b2Body_IsValid(bodyId_))
        {
            b2DestroyBody(bodyId_);
            bodyId_ = b2_nullBodyId;
            shapeId_ = b2_nullShapeId;
        }
    }

    void BoxColliderComponent::UpdateShape()
    {
        if (!b2Body_IsValid(bodyId_)) return;

        
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.isSensor = isSensor_;
        
        

        
        const Math::Transform& transform = owner_->GetTransformRef();
        float hx_m = (boxExtent_.x * transform.scale.x) / Physics::PPM;
        float hy_m = (boxExtent_.y * transform.scale.y) / Physics::PPM;
        
        
        if (hx_m < 0.01f) hx_m = 0.01f;
        if (hy_m < 0.01f) hy_m = 0.01f;

        if (massOverride_ > 0.0f)
        {
             
             
             float area = (2.0f * hx_m) * (2.0f * hy_m);
             shapeDef.density = massOverride_ / area;
        }
        else
        {
             shapeDef.density = density_;
        }

        
        b2Polygon box = b2MakeBox(hx_m, hy_m);
        shapeId_ = b2CreatePolygonShape(bodyId_, &shapeDef, &box);
        
        if (b2Shape_IsValid(shapeId_))
        {
            b2Shape_SetFriction(shapeId_, friction_);
            b2Shape_SetRestitution(shapeId_, restitution_);
        }
    }

    
    
    

    void BoxColliderComponent::SetSimulatePhysics(bool simulate)
    {
        if (simulatePhysics_ == simulate) return;
        simulatePhysics_ = simulate;
        CreatePhysicsState(); 
    }

    void BoxColliderComponent::SetBoxExtent(const Math::Vector2<float>& extent)
    {
        boxExtent_ = extent;
        CreatePhysicsState(); 
    }

    void BoxColliderComponent::SetFixedRotation(bool fixed)
    {
        fixedRotation_ = fixed;
        if (b2Body_IsValid(bodyId_))
            b2Body_SetFixedRotation(bodyId_, fixed);
    }

    void BoxColliderComponent::SetIsSensor(bool isSensor)
    {
        if (isSensor_ == isSensor) return;
        isSensor_ = isSensor;
        CreatePhysicsState();
    }

    
    
    

    void BoxColliderComponent::SetDensity(float density)
    {
        density_ = density;
        if (massOverride_ <= 0.0f && b2Shape_IsValid(shapeId_))
        {
            b2Shape_SetDensity(shapeId_, density_, true);
        }
    }

    void BoxColliderComponent::SetMass(float mass)
    {
        massOverride_ = mass;
        CreatePhysicsState(); 
    }

    void BoxColliderComponent::SetFriction(float friction)
    {
        friction_ = friction;
        if (b2Shape_IsValid(shapeId_))
            b2Shape_SetFriction(shapeId_, friction_);
    }

    void BoxColliderComponent::SetRestitution(float restitution)
    {
        restitution_ = restitution;
        if (b2Shape_IsValid(shapeId_))
            b2Shape_SetRestitution(shapeId_, restitution_);
    }

    
    
    

    void BoxColliderComponent::SetGravityScale(float scale)
    {
        gravityScale_ = scale;
        if (b2Body_IsValid(bodyId_))
        {
            b2Body_SetGravityScale(bodyId_, scale);
            b2Body_SetAwake(bodyId_, true);
        }
    }

    void BoxColliderComponent::SetAirResistance(float resistance)
    {
        airResistance_ = resistance;
    }

    void BoxColliderComponent::SetMaxFallSpeed(float speed)
    {
         maxFallSpeed_ = speed;
    }

    
    
    

    void BoxColliderComponent::SetLinearVelocity(const Math::Vector2<float>& velocity)
    {
        if (!b2Body_IsValid(bodyId_)) return;
        b2Vec2 v = { velocity.x / Physics::PPM, velocity.y / Physics::PPM };
        b2Body_SetLinearVelocity(bodyId_, v);
        b2Body_SetAwake(bodyId_, true);
    }

    Math::Vector2<float> BoxColliderComponent::GetLinearVelocity() const
    {
        if (!b2Body_IsValid(bodyId_)) return {0.0f, 0.0f};
        b2Vec2 v = b2Body_GetLinearVelocity(bodyId_);
        return { v.x * Physics::PPM, v.y * Physics::PPM };
    }

    void BoxColliderComponent::ApplyForce(const Math::Vector2<float>& force, const Math::Vector2<float>& point, bool wake)
    {
        if (!b2Body_IsValid(bodyId_)) return;
        b2Vec2 p = { point.x / Physics::PPM, point.y / Physics::PPM };
        b2Body_ApplyForce(bodyId_, {force.x, force.y}, p, wake);
    }

    void BoxColliderComponent::ApplyForceToCenter(const Math::Vector2<float>& force, bool wake)
    {
        if (!b2Body_IsValid(bodyId_)) return;
        b2Body_ApplyForceToCenter(bodyId_, {force.x, force.y}, wake);
    }

    void BoxColliderComponent::ApplyLinearImpulse(const Math::Vector2<float>& impulse, const Math::Vector2<float>& point, bool wake)
    {
        if (!b2Body_IsValid(bodyId_)) return;
        b2Vec2 p = { point.x / Physics::PPM, point.y / Physics::PPM };
        b2Body_ApplyLinearImpulse(bodyId_, {impulse.x, impulse.y}, p, wake);
    }

    void BoxColliderComponent::ApplyLinearImpulseToCenter(const Math::Vector2<float>& impulse, bool wake)
    {
        if (!b2Body_IsValid(bodyId_)) return;
        b2Body_ApplyLinearImpulseToCenter(bodyId_, {impulse.x, impulse.y}, wake);
    }

    
    
    

    void BoxColliderComponent::DispatchCollisionEnter(Actor* other, const CollisionHitResult& result)
    {
        if (onCollisionEnter_) onCollisionEnter_(other, result);
    }

    void BoxColliderComponent::DispatchCollisionExit(Actor* other, const CollisionHitResult& result)
    {
        if (onCollisionExit_) onCollisionExit_(other, result);
    }
}

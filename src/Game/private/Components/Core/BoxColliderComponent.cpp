#include "Components/Core/BoxColliderComponent.h"
#include "Framework/Actor.h"
#include "Framework/Scene.h"
#include "Math/Transform.h"


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

        if (!b2Body_IsValid(bodyId_))
            return;

        if (simulatePhysics_ && owner_)
        {
            b2Vec2 pos = b2Body_GetPosition(bodyId_);
            b2Rot rot = b2Body_GetRotation(bodyId_);
            
            Math::Vector3 worldPos(pos.x * PPM, pos.y * PPM, 0.0f);
            float worldAngleDeg = b2Rot_GetAngle(rot) * Math::Rotator::kRadiansToDegrees;

            if (Actor* parent = owner_->GetParent())
            {
                const Math::Matrix4& parentMatrix = parent->GetTransform().GetWorldMatrix();
                
                Math::Matrix4 parentInverse = parentMatrix.Inverse();
                
                Math::Vector3 localPos = parentInverse.MultiplyPoint(worldPos);
                owner_->SetPosition(localPos);

                float parentWorldYaw = parent->GetTransform().GetWorldRotation().yaw;
                owner_->SetRotation(Math::Rotator(0, worldAngleDeg - parentWorldYaw, 0));
            }
            else
            {
                owner_->SetPosition(worldPos);
                owner_->SetRotation(Math::Rotator(0, worldAngleDeg, 0));
            }
            
            ApplyAerodynamics();
            ClampTerminalVelocity();
        }
    }

    void BoxColliderComponent::DrawInspectorUI()
    {
        // Rien
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
        float maxMeters = maxFallSpeed_ / PPM;

        if (v.y > maxMeters)
        {
             v.y = maxMeters;
             b2Body_SetLinearVelocity(bodyId_, v);
        }
    }

    void BoxColliderComponent::CreatePhysicsState()
    {
        DestroyPhysicsState();
        if (!owner_)
            return;

        Scene* scene = owner_->GetOwningScene();
        if (!scene)
            return;

        b2WorldId worldId = scene->GetPhysicsWorld();
        if (!b2World_IsValid(worldId))
            return;

        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = simulatePhysics_ ? b2_dynamicBody : b2_staticBody;
        bodyDef.fixedRotation = fixedRotation_;
        bodyDef.gravityScale = gravityScale_;
        bodyDef.linearDamping = 0.0f;
        
        Math::Transform worldTrans = owner_->ComputeWorldTransform();

        bodyDef.position = { worldTrans.GetLocalPosition().x / PPM, worldTrans.GetLocalPosition().y / PPM };
        bodyDef.rotation = b2MakeRot(worldTrans.GetLocalRotation().yaw * Math::Rotator::kDegreesToRadians);
        
        bodyDef.userData = this; 

        bodyId_ = b2CreateBody(worldId, &bodyDef);
        
        if (b2Body_IsValid(bodyId_))
        {
            RecreateShape();
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

    void BoxColliderComponent::RecreateShape()
    {
        if (!b2Body_IsValid(bodyId_))
            return;

        if (b2Shape_IsValid(shapeId_))
        {
            b2DestroyShape(shapeId_, false);
        }

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.isSensor = isSensor_;
        
        if (massOverride_ > 0.0f)
        {
            
        }
        else
        {
             shapeDef.density = density_;
        }
        
        Math::Transform worldTrans = owner_->ComputeWorldTransform();
        
        float hx_m = (boxExtent_.x * worldTrans.GetLocalScale().x) / PPM;
        float hy_m = (boxExtent_.y * worldTrans.GetLocalScale().y) / PPM;
        
        if (hx_m < 0.001f)
            hx_m = 0.001f;
        
        if (hy_m < 0.001f)
            hy_m = 0.001f;

        if (massOverride_ > 0.0f)
        {
            float area = (2.0f * hx_m) * (2.0f * hy_m);
            if (area > 0.0f)
                shapeDef.density = massOverride_ / area;
        }

        b2Polygon box = b2MakeBox(hx_m, hy_m);
        shapeId_ = b2CreatePolygonShape(bodyId_, &shapeDef, &box);

        if (b2Shape_IsValid(shapeId_))
        {
            b2Shape_SetFriction(shapeId_, friction_);
            b2Shape_SetRestitution(shapeId_, restitution_);
        }
        
        if (simulatePhysics_)
        {
            b2Body_ApplyMassFromShapes(bodyId_);
        }
    }

    // --- Setters Optimisés ---

    void BoxColliderComponent::SetSimulatePhysics(bool simulate)
    {
        if (simulatePhysics_ == simulate)
            return;
        
        simulatePhysics_ = simulate;
        
        if (b2Body_IsValid(bodyId_))
        {
            b2BodyType type = simulate ? b2_dynamicBody : b2_staticBody;
            b2Body_SetType(bodyId_, type);
            
            if (simulate) b2Body_SetAwake(bodyId_, true);
        }
        else
        {
            CreatePhysicsState();
        }
    }

    void BoxColliderComponent::SetBoxExtent(const Math::Vector2& extent)
    {
        boxExtent_ = extent;

        if (b2Body_IsValid(bodyId_))
        {
            RecreateShape();
        }
    }

    void BoxColliderComponent::SetFixedRotation(bool fixed)
    {
        fixedRotation_ = fixed;
        if (b2Body_IsValid(bodyId_))
            b2Body_SetFixedRotation(bodyId_, fixed);
    }

    void BoxColliderComponent::SetIsSensor(bool isSensor)
    {
        if (isSensor_ == isSensor)
            return;
        
        isSensor_ = isSensor;
        
        if (b2Body_IsValid(bodyId_))
            RecreateShape(); 
    }

    void BoxColliderComponent::SetDensity(float density)
    {
        density_ = density;
        massOverride_ = 0.0f;

        if (b2Body_IsValid(bodyId_))
        {
            RecreateShape();
        }
    }

    void BoxColliderComponent::SetMass(float mass)
    {
        massOverride_ = mass;
        if (b2Body_IsValid(bodyId_))
        {
            RecreateShape();
        }
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

    // --- Vélocité & Impulsion ---

    void BoxColliderComponent::SetLinearVelocity(const Math::Vector2& velocity)
    {
        if (!b2Body_IsValid(bodyId_))
            return;
        
        b2Vec2 v = { velocity.x / PPM, velocity.y / PPM };
        b2Body_SetLinearVelocity(bodyId_, v);
        b2Body_SetAwake(bodyId_, true);
    }

    Math::Vector2 BoxColliderComponent::GetLinearVelocity() const
    {
        if (!b2Body_IsValid(bodyId_))
            return {0.0f, 0.0f};
        
        b2Vec2 v = b2Body_GetLinearVelocity(bodyId_);
        return { v.x * PPM, v.y * PPM };
    }

    void BoxColliderComponent::ApplyForce(const Math::Vector2& force, const Math::Vector2& point, bool wake)
    {
        if (!b2Body_IsValid(bodyId_))
            return;
        
        b2Vec2 p = { point.x / PPM, point.y / PPM };
        b2Body_ApplyForce(bodyId_, {force.x, force.y}, p, wake);
    }

    void BoxColliderComponent::ApplyForceToCenter(const Math::Vector2& force, bool wake)
    {
        if (!b2Body_IsValid(bodyId_))
            return;
        
        b2Body_ApplyForceToCenter(bodyId_, {force.x, force.y}, wake);
    }

    void BoxColliderComponent::ApplyLinearImpulse(const Math::Vector2& impulse, const Math::Vector2& point, bool wake)
    {
        if (!b2Body_IsValid(bodyId_))
            return;
        
        b2Vec2 p = { point.x / PPM, point.y / PPM };
        b2Body_ApplyLinearImpulse(bodyId_, {impulse.x, impulse.y}, p, wake);
    }

    void BoxColliderComponent::ApplyLinearImpulseToCenter(const Math::Vector2& impulse, bool wake)
    {
        if (!b2Body_IsValid(bodyId_))
            return;
        
        b2Body_ApplyLinearImpulseToCenter(bodyId_, {impulse.x, impulse.y}, wake);
    }

    // --- Dispatch ---

    void BoxColliderComponent::DispatchCollisionEnter(Actor* other, const CollisionHitResult& result)
    {
        if (onCollisionEnter_) onCollisionEnter_(other, result);
    }

    void BoxColliderComponent::DispatchCollisionExit(Actor* other, const CollisionHitResult& result)
    {
        if (onCollisionExit_) onCollisionExit_(other, result);
    }
}
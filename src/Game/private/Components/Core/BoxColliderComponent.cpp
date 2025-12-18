#include "Components/Core/BoxColliderComponent.h"
#include "Framework/Actor.h"
#include "Framework/Scene.h"
#include "Debug/Logger.h"
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

        // If simulating logic (Dynamic), we sync Physics -> Actor
        if (simulatePhysics_)
        {
            b2Vec2 pos = b2Body_GetPosition(bodyId_);
            b2Rot rot = b2Body_GetRotation(bodyId_);
            float angleRad = b2Rot_GetAngle(rot);

            if (owner_)
            {
                Math::Transform& transform = owner_->GetTransformRef();
                // We update local transform assuming parentless or relative to world?
                // Box2D is world space. BixEngine Actor Transform is local relative to parent.
                // For simplicity here, assuming root actors or handling world sync properly would require matrix math.
                // We will set local position to match world pos (assuming no parent for physics objects for now).
                
                transform.position.x = pos.x;
                transform.position.y = pos.y;
                transform.rotation.yaw = Math::Rad2Deg(angleRad);
            }
        }
        else
        {
            // If Static/Kinematic, we might want to sync Actor -> Physics if the actor moved?
            // E.g. moving platform. For now, we assume static doesn't move every frame.
            // If we wanted moving platforms, we'd use Kinematic bodies.
            // For pure Static, we just set it once on creation or via SetTransform calls (not implemented here yet).
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

        // 1. Def & Body
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = simulatePhysics_ ? b2_dynamicBody : b2_staticBody;
        bodyDef.fixedRotation = fixedRotation_;

        const Math::Transform& transform = owner_->GetTransformRef();
        bodyDef.position = { transform.position.x, transform.position.y };
        bodyDef.rotation = b2MakeRot(Math::Deg2Rad(transform.rotation.yaw));

        bodyId_ = b2CreateBody(worldId, &bodyDef);

        // 2. Shape
        if (b2Body_IsValid(bodyId_))
        {
            UpdateShape();
        }
    }

    void BoxColliderComponent::UpdateShape()
    {
        if (!b2Body_IsValid(bodyId_)) return;

        // If shape exists, destroy it? Box2D v3 allows causing destruction?
        // Or we just create a new one.
        // Assuming single shape per component for now.
        // There isn't a direct "DestroyShape" on body easily without ID.
        // If we have shapeId_, maybe we can destroy it? 
        // usage: b2DestroyShape(shapeId_); (if available in v3)
        // Let's check headers... usually yes.
        
        // Wait, v3.1 might differ. Let's try to just create a new shape and maybe leaked old one?
        // No, we should clean up. 
        // Attempt: b2DestroyShape(shapeId_) if valid.
        // But Box2D shapes are destroyed when body is destroyed.
        // If we change Extent at runtime, we might need to recreate Shape.
        
        // For safe implementation in this step: We won't support runtime shape-swapping cleanly without recreation of body
        // OR we try to destroy shape.
        
        // Let's create the shape def.
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = density_;
        // Material props set via setters later

        // Box Extent (Half-size)
        // We might also want to apply Actor Scale?
        // Unreal applies usage: Final Extent = BoxExtent * ActorScale
        const Math::Transform& transform = owner_->GetTransformRef();
        float hx = boxExtent_.x * transform.scale.x;
        float hy = boxExtent_.y * transform.scale.y;

        // Safety clamp
        if (hx < 0.1f) hx = 0.5f;
        if (hy < 0.1f) hy = 0.5f;

        b2Polygon box = b2MakeBox(hx, hy);
        // Note: Offset can be applied to b2MakeBox(hx, hy, center, angle) if we add Offset property later.

        shapeId_ = b2CreatePolygonShape(bodyId_, &shapeDef, &box);

        if (b2Shape_IsValid(shapeId_))
        {
            b2Shape_SetFriction(shapeId_, friction_);
            b2Shape_SetRestitution(shapeId_, restitution_);
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

    void BoxColliderComponent::SetSimulatePhysics(bool simulate)
    {
        if (simulatePhysics_ == simulate) return;
        simulatePhysics_ = simulate;
        UpdateBodyType();
    }

    void BoxColliderComponent::UpdateBodyType()
    {
        if (b2Body_IsValid(bodyId_))
        {
            b2Body_SetType(bodyId_, simulatePhysics_ ? b2_dynamicBody : b2_staticBody);
            
            // Should we wake it up?
            if (simulatePhysics_) b2Body_SetAwake(bodyId_, true);
        }
    }

    void BoxColliderComponent::SetBoxExtent(const Math::Vector2<float>& extent)
    {
        boxExtent_ = extent;
        // Recreate body/shape to apply size change safely for now
        CreatePhysicsState();
    }

    void BoxColliderComponent::SetDensity(float density)
    {
        density_ = density;
        if (b2Shape_IsValid(shapeId_))
        {
            b2Shape_SetDensity(shapeId_, density_, true); // Update mass
        }
    }

    void BoxColliderComponent::SetFriction(float friction)
    {
        friction_ = friction;
        if (b2Shape_IsValid(shapeId_))
        {
            b2Shape_SetFriction(shapeId_, friction_);
        }
    }

    void BoxColliderComponent::SetRestitution(float restitution)
    {
        restitution_ = restitution;
        if (b2Shape_IsValid(shapeId_))
        {
            b2Shape_SetRestitution(shapeId_, restitution_);
        }
    }

    void BoxColliderComponent::SetFixedRotation(bool fixed)
    {
        fixedRotation_ = fixed;
        if (b2Body_IsValid(bodyId_))
        {
            b2Body_SetFixedRotation(bodyId_, fixed);
        }
    }

    void BoxColliderComponent::DrawInspectorUI()
    {
        // Use Auto Inspector
    }
}

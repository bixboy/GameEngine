#pragma once
#include "Components/Core/Component.h"
#include "Math/Vector2.h"
#include <box2d/box2d.h>
#include <functional>
#include "Framework/Actor.h" 
#include "Framework/CollisionHitResult.h" 
#include "BoxColliderComponent.generated.h"

namespace BixEngine::Game
{
    BCLASS()
    class BoxColliderComponent : public Component
    {
        GENERATED_BODY()

    public:
        using Super = Component;
        using CollisionDelegate = std::function<void(Actor*, const CollisionHitResult&)>;

        BoxColliderComponent();
        explicit BoxColliderComponent(Actor* owner);
        ~BoxColliderComponent() override;

        
        
        
        void BeginPlay() override;
        void Update(float deltaTime) override;
        void DrawInspectorUI() override;
        
        [[nodiscard]] String GetTypeName() const override { return "BoxColliderComponent"; }

        
        
        
        
        
        void SetBoxExtent(const Math::Vector2<float>& extent);
        [[nodiscard]] Math::Vector2<float> GetBoxExtent() const { return boxExtent_; }

        void SetFixedRotation(bool fixed);
        [[nodiscard]] bool IsFixedRotation() const { return fixedRotation_; }

        
        void SetSimulatePhysics(bool simulate);
        [[nodiscard]] bool IsSimulatingPhysics() const { return simulatePhysics_; }



        void SetIsSensor(bool isSensor);
        [[nodiscard]] bool IsSensor() const { return isSensor_; }

        
        void SetFriction(float friction);
        [[nodiscard]] float GetFriction() const { return friction_; }

        void SetRestitution(float restitution);
        [[nodiscard]] float GetRestitution() const { return restitution_; }

        void SetDensity(float density);
        [[nodiscard]] float GetDensity() const { return density_; }

        
        
        
        
        void SetMass(float mass);
        [[nodiscard]] float GetMass() const { return massOverride_; }

        
        
        void SetAirResistance(float resistance);
        [[nodiscard]] float GetAirResistance() const { return airResistance_; }

        
        void SetGravityScale(float scale);
        [[nodiscard]] float GetGravityScale() const { return gravityScale_; }

        
        void SetMaxFallSpeed(float speed);
        [[nodiscard]] float GetMaxFallSpeed() const { return maxFallSpeed_; }


        
        
        
        
        void SetLinearVelocity(const Math::Vector2<float>& velocity);
        [[nodiscard]] Math::Vector2<float> GetLinearVelocity() const;

        
        void ApplyForce(const Math::Vector2<float>& force, const Math::Vector2<float>& point, bool wake = true);
        
        
        void ApplyForceToCenter(const Math::Vector2<float>& force, bool wake = true);

        
        void ApplyLinearImpulse(const Math::Vector2<float>& impulse, const Math::Vector2<float>& point, bool wake = true);
        
        
        void ApplyLinearImpulseToCenter(const Math::Vector2<float>& impulse, bool wake = true);


        
        
        

        template <typename T>
        void BindOnCollisionEnter(T* instance, void (T::*func)(Actor*, const CollisionHitResult&))
        {
            onCollisionEnter_ = [instance, func](Actor* other, const CollisionHitResult& result) { (instance->*func)(other, result); };
        }

        template <typename T>
        void BindOnCollisionExit(T* instance, void (T::*func)(Actor*, const CollisionHitResult&))
        {
            onCollisionExit_ = [instance, func](Actor* other, const CollisionHitResult& result) { (instance->*func)(other, result); };
        }

        void DispatchCollisionEnter(Actor* other, const CollisionHitResult& result);
        void DispatchCollisionExit(Actor* other, const CollisionHitResult& result);

    private:
        
        void CreatePhysicsState();
        void DestroyPhysicsState();
        void UpdateShape();
        
        
        void ApplyAerodynamics(); 
        void ClampTerminalVelocity();

    private:
        
        BPROPERTY(EditAnywhere, Category="Physics|State")
        bool simulatePhysics_ = false; 

        BPROPERTY(EditAnywhere, Category="Physics|State")
        bool fixedRotation_ = false;



        BPROPERTY(EditAnywhere, Category="Physics|State")
        bool isSensor_ = false;

        
        BPROPERTY(EditAnywhere, Category="Physics|Shape")
        Math::Vector2<float> boxExtent_{32.0f, 32.0f}; 

        
        BPROPERTY(EditAnywhere, Category="Physics|Material")
        float density_ = 1.0f;

        BPROPERTY(EditAnywhere, Category="Physics|Material")
        float friction_ = 0.5f;

        BPROPERTY(EditAnywhere, Category="Physics|Material")
        float restitution_ = 0.1f;

        
        BPROPERTY(EditAnywhere, Category="Physics|Dynamics")
        float massOverride_ = 0.0f; 

        BPROPERTY(EditAnywhere, Category="Physics|Dynamics")
        float airResistance_ = 0.0f;

        BPROPERTY(EditAnywhere, Category="Physics|Dynamics")
        float gravityScale_ = 1.0f;

        BPROPERTY(EditAnywhere, Category="Physics|Dynamics")
        float maxFallSpeed_ = 2000.0f; 

        
        b2BodyId bodyId_ = b2_nullBodyId;
        b2ShapeId shapeId_ = b2_nullShapeId;

        
        CollisionDelegate onCollisionEnter_;
        CollisionDelegate onCollisionExit_;
    };
}

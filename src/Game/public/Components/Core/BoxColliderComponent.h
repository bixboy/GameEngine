#include "Components/Core/Component.h"
#include "Math/Vector2.h"
#include <box2d/box2d.h>
#include "BoxColliderComponent.generated.h"

namespace BixEngine::Game
{
    BCLASS()
    class BoxColliderComponent : public Component
    {
        GENERATED_BODY()

    public:
        using Super = Component;

        BoxColliderComponent();
        explicit BoxColliderComponent(Actor* owner);
        ~BoxColliderComponent() override;

        void BeginPlay() override;
        void Update(float deltaTime) override;
        void DrawInspectorUI() override;

        [[nodiscard]] String GetTypeName() const override { return "BoxColliderComponent"; }

        // ────────────────────────────────────────────────
        // Properties
        // ────────────────────────────────────────────────
        
        // Simulating Physics means it's a Dynamic body affected by gravity and forces.
        // If false, it's a Static body (like a wall/floor).
        void SetSimulatePhysics(bool simulate);
        [[nodiscard]] bool IsSimulatingPhysics() const { return simulatePhysics_; }

        // Extents = Half-size (like Unreal). Box Width = 2 * X, Height = 2 * Y.
        void SetBoxExtent(const Math::Vector2<float>& extent);
        [[nodiscard]] Math::Vector2<float> GetBoxExtent() const { return boxExtent_; }

        void SetDensity(float density);
        [[nodiscard]] float GetDensity() const { return density_; }

        void SetFriction(float friction);
        [[nodiscard]] float GetFriction() const { return friction_; }

        void SetRestitution(float restitution);
        [[nodiscard]] float GetRestitution() const { return restitution_; }

        void SetFixedRotation(bool fixed);
        [[nodiscard]] bool IsFixedRotation() const { return fixedRotation_; }

    private:
        void CreatePhysicsState();
        void DestroyPhysicsState();
        void UpdateBodyType();
        void UpdateShape();

    private:
        // Config
        BPROPERTY(EditAnywhere, Category="Physics")
        bool simulatePhysics_ = false; 

        BPROPERTY(EditAnywhere, Category="Physics")
        bool fixedRotation_ = false;

        BPROPERTY(EditAnywhere, Category="Dimensions")
        Math::Vector2<float> boxExtent_{32.0f, 32.0f}; 

        BPROPERTY(EditAnywhere, Category="Material")
        float density_ = 1.0f;

        BPROPERTY(EditAnywhere, Category="Material")
        float friction_ = 0.5f;

        BPROPERTY(EditAnywhere, Category="Material")
        float restitution_ = 0.1f;

        // Runtime
        b2BodyId bodyId_ = b2_nullBodyId;
        b2ShapeId shapeId_ = b2_nullShapeId;
    };
}

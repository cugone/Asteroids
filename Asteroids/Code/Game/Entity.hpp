#pragma once

#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Math/Matrix4.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector4.hpp"

#include "Engine/Renderer/Mesh.hpp"

namespace a2de {
    class Renderer;
}

class Entity {
public:
    enum class Faction {
        None
        ,Player
        ,Enemy
        ,Asteroid
    };

    virtual ~Entity() = default;
    virtual void BeginFrame() noexcept;
    virtual void Update(a2de::TimeUtils::FPSeconds deltaSeconds) noexcept;
    virtual void Render(a2de::Renderer& renderer) const noexcept;
    virtual void EndFrame() noexcept;
    virtual void OnCreate() noexcept = 0;
    virtual void OnCollision(Entity* a, Entity* b) noexcept=0;
    virtual void OnFire() noexcept=0;
    virtual void OnDestroy() noexcept=0;

    void RotateCounterClockwise(float speed) noexcept;
    void RotateClockwise(float speed) noexcept;
    float GetRotationSpeed() const noexcept;
    void SetRotationSpeed(float speed) noexcept;

    a2de::Vector2 GetPosition() const noexcept;
    void SetPosition(a2de::Vector2 newPosition) noexcept;
    a2de::Vector2 GetVelocity() const noexcept;
    void SetVelocity(a2de::Vector2 newVelocity) noexcept;
    a2de::Vector2 GetAcceleration() const noexcept;

    void SetOrientationDegrees(float newDegrees) noexcept;
    float GetOrientationDegrees() const noexcept;

    float GetCosmeticRadius() const noexcept;
    float GetPhysicalRadius() const noexcept;

    float GetSpeed() const noexcept;

    void Kill() noexcept;
    bool IsDead() const noexcept;

    const a2de::Matrix4& GetTransform() const noexcept;

    a2de::Material* GetMaterial() const noexcept;

    void DecrementHealth() noexcept;

    a2de::Vector2 GetForward() const noexcept;
    a2de::Vector2 GetBackward() const noexcept;
    a2de::Vector2 GetRight() const noexcept;
    a2de::Vector2 GetLeft() const noexcept;

    long long scoreValue = 0ll;
    Faction faction = Faction::None;
protected:
    void SetHealth(int newHealth) noexcept;

    a2de::Vector2 GetForce() const noexcept;
    void AddForce(const a2de::Vector2& force) noexcept;

    void SetCosmeticRadius(float value) noexcept;
    void SetPhysicalRadius(float value) noexcept;

    a2de::Material* material{};
    a2de::Matrix4 transform{};
    a2de::Mesh::Builder mesh_builder{};
private:

    a2de::Vector2 CalcAcceleration() noexcept;
    
    void ClearForce() noexcept;
    float GetMass() const noexcept;
    float GetInvMass() const noexcept;
    
    void SetOrientationRadians(float newRadians) noexcept;
    float GetOrientationRadians() const noexcept;

    void AdjustOrientation(float value) noexcept;

    a2de::Vector4 position_orientation_speed{};
    a2de::Vector4 cosmeticphysicalradius_velocitydirection{};
    a2de::Vector4 acceleration_force{};
    a2de::Vector4 invmass_rotationspeed_health_padding{1.0f, 90.0f, 1.0f, 0.0f};
};



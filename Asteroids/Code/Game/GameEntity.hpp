#pragma once

#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Math/Matrix4.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector4.hpp"

#include "Engine/Renderer/Mesh.hpp"
#include "Engine/Scene/Entity.hpp"

class IWeapon;

class GameEntity : public Entity {
public:
    enum class Faction {
        None
        , Player
        , Enemy
        , Asteroid
    };

    GameEntity() noexcept;
    virtual ~GameEntity() = default;
    virtual void BeginFrame() noexcept;
    virtual void Update(TimeUtils::FPSeconds deltaSeconds) noexcept;
    virtual void Render() const noexcept;
    virtual void EndFrame() noexcept;
    virtual void OnCreate() noexcept = 0;
    virtual void OnCollision(GameEntity* a, GameEntity* b) noexcept = 0;
    virtual void OnFire() noexcept = 0;
    virtual void OnDestroy() noexcept = 0;

    void RotateCounterClockwise(float speed) noexcept;
    void RotateClockwise(float speed) noexcept;
    float GetRotationSpeed() const noexcept;
    void SetRotationSpeed(float speed) noexcept;

    IWeapon* GetWeapon() const noexcept;

    Vector2 GetPosition() const noexcept;
    void SetPosition(Vector2 newPosition) noexcept;
    Vector2 GetVelocity() const noexcept;
    void SetVelocity(Vector2 newVelocity) noexcept;
    Vector2 GetAcceleration() const noexcept;

    void SetOrientationDegrees(float newDegrees) noexcept;
    float GetOrientationDegrees() const noexcept;

    float GetCosmeticRadius() const noexcept;
    float GetPhysicalRadius() const noexcept;

    float GetSpeed() const noexcept;

    void Kill() noexcept;
    bool IsDead() const noexcept;

    const Matrix4& GetTransform() const noexcept;
    Matrix4& GetTransform() noexcept;

    virtual Material* GetMaterial() const noexcept = 0;

    void DecrementHealth() noexcept;

    Vector2 GetForward() const noexcept;
    Vector2 GetBackward() const noexcept;
    Vector2 GetRight() const noexcept;
    Vector2 GetLeft() const noexcept;

    long long scoreValue = 0ll;
    Faction faction = Faction::None;
protected:
    void SetHealth(int newHealth) noexcept;

    Vector2 GetForce() const noexcept;
    void AddForce(const Vector2& force) noexcept;

    void SetCosmeticRadius(float value) noexcept;
    void SetPhysicalRadius(float value) noexcept;

    IWeapon* m_weapon{};
    Mesh::Builder m_mesh_builder{};
private:

    Vector2 CalcAcceleration() noexcept;
    
    void ClearForce() noexcept;
    float GetMass() const noexcept;
    float GetInvMass() const noexcept;
    
    void SetOrientationRadians(float newRadians) noexcept;
    float GetOrientationRadians() const noexcept;

    void AdjustOrientation(float value) noexcept;

    Vector4 m_position_orientation_speed{};
    Vector4 m_cosmeticphysicalradius_velocitydirection{};
    Vector4 m_acceleration_force{};
    Vector4 m_invmass_rotationspeed_health_padding{1.0f, 90.0f, 1.0f, 0.0f};
};

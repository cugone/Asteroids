#pragma once

#include "Engine/Math/Vector2.hpp"

#include "Game/Entity.hpp"

class ThrustComponent : public Entity {
public:
    ThrustComponent() = default;
    explicit ThrustComponent(Entity* parent, float maxThrust = 100.0f);
    ThrustComponent(const ThrustComponent& other) = default;
    ThrustComponent(ThrustComponent&& other) = default;
    ThrustComponent& operator=(const ThrustComponent& other) = default;
    ThrustComponent& operator=(ThrustComponent&& other) = default;
    virtual ~ThrustComponent() = default;

    void Update([[maybe_unused]] a2de::TimeUtils::FPSeconds deltaSeconds) noexcept override;

    void OnCreate() noexcept override;
    void OnCollision(Entity* a, Entity* b) noexcept override;
    void OnFire() noexcept override;
    void OnDestroy() noexcept override;

    void SetThrust(float thrust) noexcept;
    float GetThrust(float thrust) const noexcept;
    float GetMaxThrust() const noexcept;
    void SetMaxThrust(float newMaxThrust) noexcept;

protected:
private:
    Entity* m_parent{nullptr};
    a2de::Vector2 m_positionOffset{};
    float m_thrustDirectionAngleOffset{0.0f};
    float m_thrust{0.0f};
    float m_maxThrust{100.0f};
};

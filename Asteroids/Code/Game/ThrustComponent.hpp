#pragma once

#include "Engine/Math/Vector2.hpp"

#include "Engine/Physics/Particles/ParticleEffect.hpp"

#include "Engine/Scene/Scene.hpp"

#include "Game/GameEntity.hpp"

#include <memory>

class ThrustComponent : public GameEntity {
public:
    explicit ThrustComponent(std::weak_ptr<Scene> scene, GameEntity* parent, float maxThrust = 100.0f);
    ThrustComponent(const ThrustComponent& other) = default;
    ThrustComponent(ThrustComponent&& other) = default;
    ThrustComponent& operator=(const ThrustComponent& other) = default;
    ThrustComponent& operator=(ThrustComponent&& other) = default;
    virtual ~ThrustComponent() = default;

    void BeginFrame() noexcept override;
    void Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void Render() const noexcept override;
    void EndFrame() noexcept override;

    void OnCreate() noexcept override;
    void OnCollision(GameEntity* a, GameEntity* b) noexcept override;
    void OnFire() noexcept override;
    void OnDestroy() noexcept override;

    void SetThrust(float thrust) noexcept;
    float GetThrust(float thrust) const noexcept;
    float GetMaxThrust() const noexcept;
    void SetMaxThrust(float newMaxThrust) noexcept;

    Material* GetMaterial() const noexcept override;
protected:
private:
    ParticleEffect m_thrustPS{"flame_emission"};
    GameEntity* m_parent{nullptr};
    Vector2 m_positionOffset{};
    float m_thrustDirectionAngleOffset{0.0f};
    float m_thrust{0.0f};
    float m_maxThrust{100.0f};
};

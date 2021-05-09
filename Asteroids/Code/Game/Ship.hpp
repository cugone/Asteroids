#pragma once

#include "Engine/Core/Stopwatch.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Game/Entity.hpp"

namespace a2de {
    class Renderer;
}
class ThrustComponent;
class LaserComponent;
class LaserChargeComponent;

class Ship : public Entity {
public:
    Ship();
    explicit Ship(a2de::Vector2 position);
    virtual ~Ship() = default;

    void BeginFrame() noexcept override;
    void Update(a2de::TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void Render(a2de::Renderer& renderer) const noexcept override;
    void EndFrame() noexcept override;

    void OnCreate() noexcept override;
    void OnFire() noexcept override;
    void OnCollision(Entity* a, Entity* b) noexcept override;
    void OnDestroy() noexcept override;

    void Thrust(float force) noexcept;
    void Laser() noexcept;
    void Charge() noexcept;

    void SetRespawning() noexcept;
    const bool IsRespawning() const noexcept;
    void DoneRespawning() noexcept;

    void DropMine() noexcept;

private:

    void MakeBullet() const noexcept;
    void MakeMine() const noexcept;

    void DoScaleEaseOut(a2de::TimeUtils::FPSeconds& deltaSeconds) noexcept;
    float DoAlphaEaseOut(a2de::TimeUtils::FPSeconds& deltaSeconds) const noexcept;

    const a2de::Vector2 CalcBulletDirectionFromDifficulty() const noexcept;
    const a2de::Vector2 CalcNewBulletVelocity() const noexcept;
    const a2de::Vector2 CalcNewBulletPosition() const noexcept;

    std::unique_ptr<ThrustComponent> _thrust{};
    std::unique_ptr<LaserComponent> _laser{};
    std::unique_ptr<LaserChargeComponent> _laserCharge{};
    a2de::Stopwatch _fireRate;
    a2de::Stopwatch _mineFireRate;
    const float _bulletSpeed{400.0f};
    float _maxScale{2.0f};
    float _scale{1.0f};
    bool _canFire = false;
    bool _canDropMine = false;
    bool _respawning = true;
};

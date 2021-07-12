#pragma once

#include "Engine/Core/Stopwatch.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Game/Entity.hpp"
#include "Game/LaserBulletWeapon.hpp"

class Renderer;
class ThrustComponent;

class Ship : public Entity {
public:
    Ship();
    explicit Ship(Vector2 position);
    virtual ~Ship() = default;

    void BeginFrame() noexcept override;
    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void Render() const noexcept override;
    void EndFrame() noexcept override;

    void OnCreate() noexcept override;
    void OnFire() noexcept override;
    void OnCollision(Entity* a, Entity* b) noexcept override;
    void OnDestroy() noexcept override;

    void Thrust(float force) noexcept;

    void SetRespawning() noexcept;
    const bool IsRespawning() const noexcept;
    void DoneRespawning() noexcept;

    void DropMine() noexcept;

private:

    void MakeBullet() const noexcept;
    void MakeMine() const noexcept;

    void DoScaleEaseOut(TimeUtils::FPSeconds& deltaSeconds) noexcept;
    float DoAlphaEaseOut(TimeUtils::FPSeconds& deltaSeconds) const noexcept;

    const Vector2 CalcBulletDirectionFromDifficulty() const noexcept;
    const Vector2 CalcNewBulletVelocity() const noexcept;
    const Vector2 CalcNewBulletPosition() const noexcept;

    std::unique_ptr<ThrustComponent> _thrust{};
    Stopwatch _mineFireRate;
    LaserBulletWeapon _laserWeapon{};
    float _maxScale{2.0f};
    float _scale{1.0f};
    bool _canDropMine = false;
    bool _respawning = true;
};

#pragma once

#include "Game/IWeapon.hpp"

#include "Engine/Core/Stopwatch.hpp"

class LaserBulletWeapon : public IWeapon {
public:
    virtual ~LaserBulletWeapon() noexcept = default;

    void Initialize(const WeaponDesc& desc) noexcept override;
    void Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept override;

    bool Fire() noexcept override;

    Material* GetMaterial() const noexcept override;
    float GetFireRate() const noexcept override;
    void SetFireRate(float newFireRate) noexcept override;
    TimeUtils::FPMilliseconds GetFireDelay() const noexcept override;
    void SetFireDelay(TimeUtils::FPMilliseconds newFireDelay) noexcept override;
    float GetScale() const noexcept override;
    void SetScale(float newScale) noexcept override;
    float GetSpeed() const noexcept override;
    void SetSpeed(float newSpeed) noexcept override;

protected:
private:
    WeaponDesc m_desc{};
    Stopwatch m_fireDelay{};
    Stopwatch m_fireRate{};
    bool m_canFire{false};
    bool m_canSpawnBullet{false};
};

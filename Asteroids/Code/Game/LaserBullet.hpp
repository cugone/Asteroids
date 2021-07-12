#pragma once

#include "Game/IWeapon.hpp"

class LaserBullet : public IWeapon {
public:
    virtual ~LaserBullet() noexcept = default;

    void Initialize(const WeaponDesc& desc) override;
    void BeginFrame() noexcept override;
    void Update(TimeUtils::FPSeconds deltaTime) noexcept override;
    void Render() const noexcept override;
    void EndFrame() noexcept override;
    Material* GetMaterial() const noexcept override;
    void SetMaterial(Material* newMaterial) noexcept override;
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
    
};

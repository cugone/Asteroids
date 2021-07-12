#pragma once

#include "Engine/Core/TimeUtils.hpp"

class Material;

struct WeaponDesc {
    float fireRate{1.0f};
    TimeUtils::FPMilliseconds fireDelay{0.0f};
    float scale{1.0f};
    float bulletSpeed{1.0f};
    std::string materialName{};
};

class IWeapon {
public:
    virtual ~IWeapon() noexcept = default;

    virtual void Initialize(const WeaponDesc& desc) noexcept = 0;
    virtual void Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept = 0;

    virtual bool Fire() noexcept = 0;
    virtual Material* GetMaterial() const noexcept = 0;

    virtual float GetFireRate() const noexcept = 0;
    virtual void SetFireRate(float newFireRate) noexcept = 0;
    
    virtual TimeUtils::FPMilliseconds GetFireDelay() const noexcept = 0;
    virtual void SetFireDelay(TimeUtils::FPMilliseconds newFireDelay) noexcept = 0;

    virtual float GetScale() const noexcept = 0;
    virtual void SetScale(float newScale) noexcept = 0;

    virtual float GetSpeed() const noexcept = 0;
    virtual void SetSpeed(float newSpeed) noexcept = 0;

protected:
private:
    
};

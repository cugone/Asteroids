#include "Game/LaserBulletWeapon.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

void LaserBulletWeapon::Initialize(const WeaponDesc& desc) noexcept {
    m_desc = desc;
    m_fireDelay.SetSeconds(m_desc.fireDelay);
    m_fireRate.SetSeconds(TimeUtils::FPSeconds{m_desc.fireRate});
}

void LaserBulletWeapon::Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    m_canFire = m_fireDelay.CheckAndReset();
    m_canSpawnBullet = m_fireRate.Check();
}

bool LaserBulletWeapon::Fire() noexcept {
    m_canSpawnBullet = m_fireRate.CheckAndReset();
    return m_canSpawnBullet && m_canFire;
}

Material* LaserBulletWeapon::GetMaterial() const noexcept {
    auto* rs = ServiceLocator::get<IRendererService>();
    return rs->GetMaterial(m_desc.materialName);
}

float LaserBulletWeapon::GetFireRate() const noexcept {
    return m_desc.fireRate;
}

void LaserBulletWeapon::SetFireRate(float newFireRate) noexcept {
    m_desc.fireRate = newFireRate;
}

TimeUtils::FPMilliseconds LaserBulletWeapon::GetFireDelay() const noexcept {
    return m_desc.fireDelay;
}

void LaserBulletWeapon::SetFireDelay(TimeUtils::FPMilliseconds newFireDelay) noexcept {
    m_desc.fireDelay = newFireDelay;
}

float LaserBulletWeapon::GetScale() const noexcept {
    return m_desc.scale;
}

void LaserBulletWeapon::SetScale(float newScale) noexcept {
    m_desc.scale = newScale;
}

float LaserBulletWeapon::GetSpeed() const noexcept {
    return m_desc.bulletSpeed;
}

void LaserBulletWeapon::SetSpeed(float newSpeed) noexcept {
    m_desc.bulletSpeed = newSpeed;
}


#include "Game/LaserBullet.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

void LaserBullet::Initialize(const WeaponDesc& desc) {
    /* DO NOTHING */
}

void LaserBullet::BeginFrame() noexcept {
    /* DO NOTHING */
}

void LaserBullet::Update(TimeUtils::FPSeconds deltaTime) noexcept {
    /* DO NOTHING */
}

void LaserBullet::Render() const noexcept {
    /* DO NOTHING */
}

void LaserBullet::EndFrame() noexcept {
    /* DO NOTHING */
}

Material* LaserBullet::GetMaterial() const noexcept {
    auto& rs = ServiceLocator::get<IRendererService>();
    return 
}

void LaserBullet::SetMaterial(Material* newMaterial) noexcept {

}

float LaserBullet::GetFireRate() const noexcept {
    /* DO NOTHING */
}

void LaserBullet::SetFireRate(float newFireRate) noexcept {
    /* DO NOTHING */
}

TimeUtils::FPMilliseconds LaserBullet::GetFireDelay() const noexcept {
    /* DO NOTHING */
}

void LaserBullet::SetFireDelay(TimeUtils::FPMilliseconds newFireDelay) noexcept {
    /* DO NOTHING */
}

float LaserBullet::GetScale() const noexcept {
    /* DO NOTHING */
}

void LaserBullet::SetScale(float newScale) noexcept {
    /* DO NOTHING */
}

float LaserBullet::GetSpeed() const noexcept {
    /* DO NOTHING */
}

void LaserBullet::SetSpeed(float newSpeed) noexcept {
    /* DO NOTHING */
}


#pragma once

#include "Engine/Core/Stopwatch.hpp"

#include "Engine/Math/Ray2.hpp"
#include "Engine/Math/Vector2.hpp"

#include "Game/Entity.hpp"

class LaserChargeComponent : public Entity {
public:
    LaserChargeComponent() = delete;
    explicit LaserChargeComponent(Entity * parent);
    LaserChargeComponent(const LaserChargeComponent& other) = delete;
    LaserChargeComponent(LaserChargeComponent&& other) = delete;
    LaserChargeComponent& operator=(const LaserChargeComponent& other) = delete;
    LaserChargeComponent& operator=(LaserChargeComponent&& other) = delete;
    virtual ~LaserChargeComponent() = default;

    void BeginFrame() noexcept override;
    void Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void EndFrame() noexcept override;

    void OnDestroy() noexcept override;
    void OnFire() noexcept override;
    void OnCollision(Entity* a, Entity* b) noexcept override;
    void OnCreate() noexcept override;

    bool DoneFiring() noexcept;

protected:
private:
    Entity* m_parent{nullptr};
    std::unique_ptr<AnimatedSprite> _sprite{};
    Vector2 m_positionOffset{};
    Ray2 m_ray{};
    bool m_ischarging{true};
    Stopwatch m_chargeTime{TimeUtils::FPSeconds{1.0f}};
};

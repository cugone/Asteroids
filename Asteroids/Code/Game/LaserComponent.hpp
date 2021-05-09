#pragma once

#include "Engine/Core/Stopwatch.hpp"

#include "Engine/Math/Ray2.hpp"
#include "Engine/Math/Vector2.hpp"

#include "Game/Entity.hpp"

class LaserComponent : public Entity {
public:
    LaserComponent() = delete;
    explicit LaserComponent(Entity * parent);
    LaserComponent(const LaserComponent& other) = delete;
    LaserComponent(LaserComponent&& other) = delete;
    LaserComponent& operator=(const LaserComponent& other) = delete;
    LaserComponent& operator=(LaserComponent&& other) = delete;
    virtual ~LaserComponent() = default;

    void BeginFrame() noexcept override;
    void Update([[maybe_unused]] a2de::TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void EndFrame() noexcept override;

    void OnDestroy() noexcept override;
    void OnFire() noexcept override;
    void OnCollision(Entity* a, Entity* b) noexcept override;
    void OnCreate() noexcept override;

    bool DoneFiring() noexcept;

protected:
private:
    Entity* m_parent{nullptr};
    a2de::Vector2 m_positionOffset{};
    a2de::Ray2 m_ray{};
    a2de::Stopwatch m_fireTime{a2de::TimeUtils::FPSeconds{5.0f}};
    bool m_isfiring{true};
};

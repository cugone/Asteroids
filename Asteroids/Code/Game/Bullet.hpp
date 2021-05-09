#pragma once

#include "Engine/Core/Stopwatch.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Math/Vector2.hpp"

#include "Game/Entity.hpp"

class Renderer;

class Bullet : public Entity {
public:
    Bullet() = default;
    explicit Bullet(const Entity* parent, a2de::Vector2 position, a2de::Vector2 velocity) noexcept;
    virtual ~Bullet() = default;
    void Update(a2de::TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void EndFrame() noexcept override;

    void OnCreate() noexcept override;
    void OnFire() noexcept override;
    void OnCollision(Entity* a, Entity* b) noexcept override;
    void OnDestroy() noexcept override;

private:
    float CalculateTtlFromDifficulty() const noexcept;

    const Entity* _parent{};
    a2de::Stopwatch ttl{};
};


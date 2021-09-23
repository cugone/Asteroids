#pragma once

#include "Engine/Core/Stopwatch.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Math/Vector2.hpp"

#include "Game/GameEntity.hpp"

class Renderer;

class Bullet : public GameEntity {
public:
    explicit Bullet(const GameEntity* parent, Vector2 position, Vector2 velocity) noexcept;
    virtual ~Bullet() = default;
    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void EndFrame() noexcept override;

    void OnCreate() noexcept override;
    void OnFire() noexcept override;
    void OnCollision(GameEntity* a, GameEntity* b) noexcept override;
    void OnDestroy() noexcept override;

    Material* GetMaterial() const noexcept override;
private:
    float CalculateTtlFromDifficulty() const noexcept;

    const GameEntity* _parent{};
    Stopwatch ttl{};
};


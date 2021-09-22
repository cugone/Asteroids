#pragma once

#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"

#include "Game/GameEntity.hpp"

#include <memory>
#include <utility>

class Renderer;

class Explosion : public GameEntity {
public:
    explicit Explosion(Vector2 position);

    virtual ~Explosion() = default;
    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void EndFrame() noexcept override;

    void OnCreate() noexcept override;
    void OnFire() noexcept override;
    void OnCollision(GameEntity* a, GameEntity* b) noexcept override;
    void OnDestroy() noexcept override;

    Material* GetMaterial() const noexcept override;
private:
    std::unique_ptr<AnimatedSprite> _sprite{};
};



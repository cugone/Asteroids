#pragma once

#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"

#include "Game/Entity.hpp"

#include <memory>
#include <utility>

class Renderer;

class Explosion : public Entity {
public:
    explicit Explosion(a2de::Vector2 position);

    virtual ~Explosion() = default;
    void Update(a2de::TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void EndFrame() noexcept override;

    void OnCreate() noexcept override;
    void OnFire() noexcept override;
    void OnCollision(Entity* a, Entity* b) noexcept override;
    void OnDestroy() noexcept override;

private:
    std::unique_ptr<a2de::AnimatedSprite> _sprite{};
};



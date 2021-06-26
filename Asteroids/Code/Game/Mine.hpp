#pragma once

#include "Engine/Math/Vector2.hpp"

#include "Game/Entity.hpp"

class Mine : public Entity {
public:
    explicit Mine(const Entity* parent, Vector2 position);
    virtual ~Mine() = default;

    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void EndFrame() noexcept override;

    void OnCreate() noexcept override;
    void OnCollision(Entity* a, Entity* b) noexcept override;
    void OnFire() noexcept override;
    void OnDestroy() noexcept;

protected:
private:
    std::unique_ptr<class AnimatedSprite> _sprite{};
    std::weak_ptr<class SpriteSheet> GetSpriteSheet() const noexcept;
};

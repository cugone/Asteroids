#pragma once

#include "Engine/Math/Vector2.hpp"

#include "Game/Entity.hpp"

class Mine : public Entity {
public:
    explicit Mine(const Entity* parent, a2de::Vector2 position);
    virtual ~Mine() = default;

    void Update(a2de::TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void EndFrame() noexcept override;

    void OnCreate() noexcept override;
    void OnCollision(Entity* a, Entity* b) noexcept override;
    void OnFire() noexcept override;
    void OnDestroy() noexcept;

protected:
private:
    std::unique_ptr<a2de::AnimatedSprite> _sprite{};
    std::weak_ptr<a2de::SpriteSheet> GetSpriteSheet() const noexcept;
};

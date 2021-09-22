#pragma once

#include "Engine/Math/Vector2.hpp"

#include "Game/GameEntity.hpp"

class Mine : public GameEntity {
public:
    explicit Mine(const GameEntity* parent, Vector2 position);
    virtual ~Mine() = default;

    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void EndFrame() noexcept override;

    void OnCreate() noexcept override;
    void OnCollision(GameEntity* a, GameEntity* b) noexcept override;
    void OnFire() noexcept override;
    void OnDestroy() noexcept;

    Material* GetMaterial() const noexcept override;
protected:
private:
    std::unique_ptr<class AnimatedSprite> _sprite{};
    std::weak_ptr<class SpriteSheet> GetSpriteSheet() const noexcept;
};

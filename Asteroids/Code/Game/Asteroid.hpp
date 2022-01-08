#pragma once

#include "Engine/Core/TimeUtils.hpp"

#include "Game/Entity.hpp"

#include <memory>
#include <tuple>
#include <utility>

class Renderer;
class ConstantBuffer;

class Asteroid : public Entity {
public:
    enum class Type {
        Large,
        Medium,
        Small
    };
    static constexpr float largeAsteroidCosmeticSize = 50.0f;
    static constexpr float largeAsteroidPhysicalSize = 40.0f;
    static constexpr float mediumAsteroidCosmeticSize = 25.0f;
    static constexpr float mediumAsteroidPhysicalSize = 20.0f;
    static constexpr float smallAsteroidCosmeticSize = 12.0f;
    static constexpr float smallAsteroidPhysicalSize = 10.0f;

    explicit Asteroid(Vector2 position, Vector2 velocity, float rotationSpeed);
    explicit Asteroid(Type type, Vector2 position, Vector2 velocity, float rotationSpeed);

    virtual ~Asteroid() = default;

    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void Render() const noexcept override;
    void EndFrame() noexcept override;

    void OnCreate() noexcept override;
    void OnFire() noexcept override;
    void OnCollision(Entity* a, Entity* b) noexcept override;
    void OnDestroy() noexcept override;

    Material* GetMaterial() const noexcept override;
private:
    void OnHit() noexcept;
    Vector4 WasHit() const noexcept;
    void MakeChildAsteroid() const noexcept;
    long long GetScoreFromType(Type type);
    std::pair<float, float> GetRadiiFromType(Type type) const noexcept;
    int GetHealthFromType(Type type) const noexcept;
    float CalcChildHeadingFromDifficulty() const noexcept;
    float CalcChildSpeedFromSizeAndDifficulty() const noexcept;
    float CalcChildSpeedFromSize() const noexcept;
    std::tuple<Vector2, Vector2, float> CalcChildPhysicsParameters() const noexcept;

    struct asteroid_state_t {
        Vector4 wasHit{};
    };

    ConstantBuffer* asteroid_state_cb{nullptr};
    mutable asteroid_state_t asteroid_state{};
    Type _type{Type::Large};
    std::unique_ptr<class AnimatedSprite> _sprite{};
    TimeUtils::FPSeconds _timeSinceLastHit{0.0f};
};



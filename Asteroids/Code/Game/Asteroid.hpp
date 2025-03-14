#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Scene/Scene.hpp"

#include "Game/GameEntity.hpp"

#include <memory>
#include <tuple>
#include <utility>

class Renderer;
class ConstantBuffer;

class Asteroid : public GameEntity {
public:
    enum class Type {
        Large,
        Medium,
        Small
    };
    static inline constexpr const float largeAsteroidCosmeticSize{50.0f};
    static inline constexpr const float largeAsteroidPhysicalSize{40.0f};
    static inline constexpr const float mediumAsteroidCosmeticSize{25.0f};
    static inline constexpr const float mediumAsteroidPhysicalSize{20.0f};
    static inline constexpr const float smallAsteroidCosmeticSize{12.0f};
    static inline constexpr const float smallAsteroidPhysicalSize{10.0f};

    static inline constexpr const unsigned long long largeAsteroidScoreValue{25LL};
    static inline constexpr const unsigned long long mediumAsteroidScoreValue{50LL};
    static inline constexpr const unsigned long long smallAsteroidScoreValue{100LL};

    explicit Asteroid(std::weak_ptr<Scene> scene, Vector2 position, Vector2 velocity, float rotationSpeed);
    explicit Asteroid(std::weak_ptr<Scene> scene, Type type, Vector2 position, Vector2 velocity, float rotationSpeed);

    virtual ~Asteroid() = default;

    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void Render() const noexcept override;
    void EndFrame() noexcept override;

    void OnCreate() noexcept override;
    void OnFire() noexcept override;
    void OnCollision(GameEntity* a, GameEntity* b) noexcept override;
    void OnDestroy() noexcept override;

    Material* GetMaterial() const noexcept override;

    static constexpr long long GetScoreFromType(Type type) noexcept {
        switch(type) {
        case Type::Large: return largeAsteroidScoreValue;
        case Type::Medium: return mediumAsteroidScoreValue;
        case Type::Small: return smallAsteroidScoreValue;
        default: return 0LL;
        }
    }

    static constexpr std::pair<const float, const float> GetRadiiFromType(Type type) noexcept {
        switch(type) {
        case Type::Large:
            return std::make_pair(largeAsteroidCosmeticSize, largeAsteroidPhysicalSize);
        case Type::Medium:
            return std::make_pair(mediumAsteroidCosmeticSize, mediumAsteroidPhysicalSize);
        case Type::Small:
            return std::make_pair(smallAsteroidCosmeticSize, smallAsteroidPhysicalSize);
        default:
            return std::make_pair(largeAsteroidCosmeticSize, largeAsteroidPhysicalSize);
        }
    }
private:
    void OnHit() noexcept;
    Vector4 WasHit() const noexcept;
    void MakeChildAsteroid() const noexcept;
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
    TimeUtils::FPSeconds _timeSinceLastHit{0.0f};
    std::unique_ptr<class AnimatedSprite> _sprite{};
};



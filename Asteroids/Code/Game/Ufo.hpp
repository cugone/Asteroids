#pragma once

#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Stopwatch.hpp"

#include "Engine/Scene/Scene.hpp"

#include "Game/GameEntity.hpp"

#include <memory>

class ConstantBuffer;

class Ufo : public GameEntity {
public:

    enum class Type {
        Small
        ,Big
        ,Boss
        ,Max_
    };

    enum class Style {
        First_Small
        ,Blue = First_Small
        ,Green
        ,Yellow
        ,Last_Small = Yellow
        ,First_Big
        ,Cyan = First_Big
        ,Magenta
        ,Last_Big = Magenta
        ,First_Boss
        ,Orange
        ,Last_Boss = Orange
    };

    Ufo(std::weak_ptr<Scene> scene, Ufo::Type type, Vector2 position);
    virtual ~Ufo() = default;

    void BeginFrame() noexcept override;
    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void Render() const noexcept override;
    void EndFrame() noexcept override;

    void OnCreate() noexcept override;
    void OnCollision(GameEntity* a, GameEntity* b) noexcept override;
    void OnHit() noexcept;
    void OnFire() noexcept override;
    void OnDestroy() noexcept override;

    static float GetCosmeticRadiusFromType(Type type) noexcept;
    static float GetPhysicalRadiusFromType(Type type) noexcept;
    static float GetScaleFromType(Type type) noexcept;
    static Style GetStyleFromType(Type type) noexcept;
    static int GetStartIndexFromTypeAndStyle(Type type, Style style) noexcept;
    static int GetFrameLengthFromTypeAndStyle(Type type, Style style) noexcept;
    static long long GetValueFromType(Type type) noexcept;
    static unsigned int GetFireRateFromTypeAndDifficulty(Type type) noexcept;
    static float GetUfoIndexFromStyle(Style style) noexcept;
    static int GetHealthFromType(Type type) noexcept;
    Material* GetMaterial() const noexcept override;
protected:

    float GetBulletSpeedFromTypeAndDifficulty(Type type) const noexcept;

    std::weak_ptr<class SpriteSheet> GetSpriteSheet() const noexcept;
    Vector2 CalculateFireTarget() const noexcept;

    float WasHit() const noexcept;

    void MakeBullet() const noexcept;

    struct ufo_state_t {
        Vector4 wasHitUfoIndex = Vector4::Y_Axis;
    };

    ConstantBuffer* ufo_state_cb{nullptr};
    mutable ufo_state_t ufo_state{};
    Type _type{Type::Small};
    Style _style{Style::Blue};
    std::unique_ptr<class AnimatedSprite> _sprite{};
    TimeUtils::FPSeconds _timeSinceLastHit{0.0f};
    Stopwatch _fireRate{};
    AudioSystem::Sound* _warble_sound{};
    Vector2 _fireTarget{};
    float _bulletSpeed{800.0f};
    bool _canFire{false};

private:

};

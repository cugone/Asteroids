#pragma once

#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Stopwatch.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"

#include "Game/Entity.hpp"

class Ufo : public Entity {
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

    Ufo(Ufo::Type type, Vector2 position);
    virtual ~Ufo() = default;

    void BeginFrame() noexcept override;
    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void Render(Renderer& renderer) const noexcept override;
    void EndFrame() noexcept override;

    void OnCreate() noexcept override;
    void OnCollision(Entity* a, Entity* b) noexcept override;
    void OnFire() noexcept override;
    void OnDestroy() noexcept override;

protected:
private:
    float GetCosmeticRadiusFromType(Type type) const noexcept;
    float GetPhysicalRadiusFromType(Type type) const noexcept;

    Style GetStyleFromType(Type type) const noexcept;
    int GetStartIndexFromTypeAndStyle(Type type, Style style) const noexcept;
    int GetFrameLengthFromTypeAndStyle(Type type, Style style) const noexcept;
    long long GetValueFromType(Type type) const noexcept;
    
    std::weak_ptr<SpriteSheet> GetSpriteSheetFromType(Type type) const noexcept;
    unsigned int GetFireRateFromTypeAndDifficulty(Type type) const noexcept;
    float GetBulletSpeedFromTypeAndDifficulty(Type type) const noexcept;
    Vector2 CalculateFireTarget() const noexcept;

    Vector4 WasHit() const noexcept;

    void MakeBullet() const noexcept;

    struct ufo_state_t {
        Vector4 wasHitUfoIndex = Vector4::Y_AXIS;
    };

    ConstantBuffer* ufo_state_cb{nullptr};
    mutable ufo_state_t ufo_state{};
    Type _type{Type::Small};
    Style _style{Style::Blue};
    std::unique_ptr<AnimatedSprite> _sprite{};
    TimeUtils::FPSeconds _timeSinceLastHit{0.0f};
    Stopwatch _fireRate{};
    AudioSystem::Sound* _warble_sound{};
    Vector2 _fireTarget{};
    float _bulletSpeed{800.0f};
    bool _canFire{false};
};

#include "Game/Ufo.hpp"

#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Shader.hpp"

#include "Game/Bullet.hpp"
#include "Game/Mine.hpp"
#include "Game/Game.hpp"
#include "Game/Ship.hpp"

#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"

Ufo::Ufo(Type type, a2de::Vector2 position)
    : Entity()
    , _type(type)
{
    SetOrientationDegrees(-90.0f);
    SetCosmeticRadius(GetCosmeticRadiusFromType(_type));
    SetPhysicalRadius(GetPhysicalRadiusFromType(_type));
    SetPosition(position);
    SetVelocity(a2de::Vector2::X_AXIS * 100.0f);
    SetHealth(GetHealthFromType(_type));
    faction = Entity::Faction::Enemy;
    _bulletSpeed = GetBulletSpeedFromTypeAndDifficulty(_type);
    _fireRate.SetFrequency(GetFireRateFromTypeAndDifficulty(_type));
    _style = GetStyleFromType(_type);
    scoreValue = GetValueFromType(_type);

    a2de::AnimatedSpriteDesc desc{};
    desc.material = g_theRenderer->GetMaterial("ufo");
    desc.spriteSheet = GetSpriteSheet();
    desc.durationSeconds = a2de::TimeUtils::FPSeconds{0.3f};
    desc.playbackMode = a2de::AnimatedSprite::SpriteAnimMode::Looping;
    desc.frameLength = GetFrameLengthFromTypeAndStyle(_type, _style);
    desc.startSpriteIndex = GetStartIndexFromTypeAndStyle(_type, _style);

    material = desc.material;

    if(auto cbs = material->GetShader()->GetConstantBuffers(); !cbs.empty()) {
        ufo_state_cb = &cbs[0].get();
    }

    _sprite = g_theRenderer->CreateAnimatedSprite(desc);

}

void Ufo::BeginFrame() noexcept {
    Entity::BeginFrame();
    _fireTarget = CalculateFireTarget();
    if(_fireRate.CheckAndReset()) {
        _canFire = true;
    }
}

void Ufo::Update(a2de::TimeUtils::FPSeconds deltaSeconds) noexcept {
    Entity::Update(deltaSeconds);
    _timeSinceLastHit += deltaSeconds;
    _sprite->Update(deltaSeconds);

    if(_canFire) {
        OnFire();
    }

    const auto uvs = _sprite->GetCurrentTexCoords();
    const auto frameWidth = static_cast<float>(_sprite->GetFrameDimensions().x);
    const auto frameHeight = static_cast<float>(_sprite->GetFrameDimensions().y);
    const auto half_extents = a2de::Vector2{frameWidth, frameHeight};
    const auto scale = GetScaleFromType(_type);
    {
        const auto S = a2de::Matrix4::CreateScaleMatrix(scale * half_extents);
        const auto R = a2de::Matrix4::Create2DRotationDegreesMatrix(90.0f + GetOrientationDegrees());
        const auto T = a2de::Matrix4::CreateTranslationMatrix(GetPosition());
        transform = a2de::Matrix4::MakeSRT(S, R, T);
    }

    auto& builder = mesh_builder;
    builder.Begin(a2de::PrimitiveType::Triangles);
    builder.SetColor(a2de::Rgba::White);

    builder.SetUV(a2de::Vector2{uvs.maxs.x, uvs.maxs.y});
    builder.AddVertex(a2de::Vector2{+0.5f, +0.5f});

    builder.SetUV(a2de::Vector2{uvs.mins.x, uvs.maxs.y});
    builder.AddVertex(a2de::Vector2{-0.5f, +0.5f});

    builder.SetUV(a2de::Vector2{uvs.mins.x, uvs.mins.y});
    builder.AddVertex(a2de::Vector2{-0.5f, -0.5f});

    builder.SetUV(a2de::Vector2{uvs.maxs.x, uvs.mins.y});
    builder.AddVertex(a2de::Vector2{+0.5f, -0.5f});

    builder.AddIndicies(a2de::Mesh::Builder::Primitive::Quad);
    builder.End(material);

}

void Ufo::Render(a2de::Renderer& renderer) const noexcept {
    ufo_state.wasHitUfoIndex.x = WasHit();
    ufo_state.wasHitUfoIndex.y = GetUfoIndexFromStyle(_style);
    ufo_state_cb->Update(*renderer.GetDeviceContext(), &ufo_state);
    Entity::Render(renderer);
}

void Ufo::EndFrame() noexcept {
    if(const auto& found = std::find(std::begin(g_theGame->ufos), std::end(g_theGame->ufos), this);
        (found != std::end(g_theGame->ufos) &&
            (*found)->IsDead()))
    {
        *found = nullptr;
    }
    Entity::EndFrame();
}

void Ufo::OnCreate() noexcept {
    a2de::AudioSystem::SoundDesc desc{};
    desc.volume = 1.0f;
    desc.frequency = 1.0f;
    desc.loopCount = -1;
    _warble_sound = g_theAudioSystem->CreateSound(g_sound_warblepath);
    //g_theAudioSystem->Play(*_warble_sound, desc);
}

void Ufo::OnCollision(Entity* a, Entity* b) noexcept {
    if(a->faction == b->faction) {
        return;
    }
    switch(b->faction) {
    case Entity::Faction::Player:
    {
        if(const auto* asBullet = dynamic_cast<Bullet*>(b); asBullet != nullptr) {
            a->DecrementHealth();
            OnHit();
        }
        if(const auto* asMine = dynamic_cast<Mine*>(b); asMine != nullptr) {
            a->Kill();
        }
        break;
    }
    default:
        break;
    }
}

void Ufo::OnFire() noexcept {
    if(_canFire) {
        _canFire = false;
        MakeBullet();
    }
}

void Ufo::OnHit() {
    if(a2de::TimeUtils::FPFrames{1.0f} < _timeSinceLastHit) {
        _timeSinceLastHit = _timeSinceLastHit.zero();
    }
    g_theAudioSystem->Play(g_sound_hitpath);
    ufo_state.wasHitUfoIndex.x = WasHit();
}

void Ufo::OnDestroy() noexcept {
    //_warble_sound->Stop();
    Entity::OnDestroy();
    g_theGame->MakeExplosion(GetPosition());
}

float Ufo::WasHit() const noexcept {
    return _timeSinceLastHit.count() == 0.0f ? 1.0f : 0.0f;
}

void Ufo::MakeBullet() const noexcept {
    const auto source = GetPosition();
    const auto angle = (_fireTarget - source).CalcHeadingDegrees();
    g_theGame->MakeBullet(this, source, a2de::Vector2::CreateFromPolarCoordinatesDegrees(_bulletSpeed, angle));
}

float Ufo::GetCosmeticRadiusFromType(Type type) noexcept {
    switch(type) {
    case Type::Small: return 15.0f;
    case Type::Big: return 30.0f;
    case Type::Boss: return 60.0f;
    default: return 15.0f;
    }
}

float Ufo::GetPhysicalRadiusFromType(Type type) noexcept {
    const auto cr = GetCosmeticRadiusFromType(type);
    switch(type) {
    case Type::Small: return 10.0f;
    case Type::Big: return  20.0f;
    case Type::Boss: return  30.0f;
    default: return cr;
    }
}

float Ufo::GetScaleFromType(Type type) noexcept {
    switch(type) {
    case Type::Small: return 1.0f;
    case Type::Big: return  2.0f;
    case Type::Boss: return  4.0f;
    default: return 1.0f;
    }
}

std::weak_ptr<a2de::SpriteSheet> Ufo::GetSpriteSheet() const noexcept {
    return g_theGame->ufo_sheet;
}

unsigned int Ufo::GetFireRateFromTypeAndDifficulty(Type type) noexcept {
    switch(type) {
    case Type::Small: return 2u;
    case Type::Big: return 1u;
    case Type::Boss: return 3u;
    default: return 0u;
    }
}

float Ufo::GetBulletSpeedFromTypeAndDifficulty(Type type) const noexcept {
    const auto typeMultiplier = [type]()->float {
        switch(type) {
        case Type::Small: return 2.0f;
        case Type::Big: return 1.0f;
        case Type::Boss: return 3.0f;
        default: return 1.0f;
        }
    }();
    const auto difficultyMultiplier = []() -> float{
        switch(g_theGame->gameOptions.difficulty) {
        case a2de::Difficulty::Easy: return 0.50f;
        case a2de::Difficulty::Normal: return 1.0f;
        case a2de::Difficulty::Hard: return 2.0f;
        default: return 1.0f;
        };
    }();
    return difficultyMultiplier * typeMultiplier * _bulletSpeed;
}

a2de::Vector2 Ufo::CalculateFireTarget() const noexcept {
    switch(_type) {
    case Type::Small: {
        if(auto ship = g_theGame->GetShip()) {
            return ship->GetPosition();
        } else {
            return GetPosition() + a2de::Vector2::CreateFromPolarCoordinatesDegrees(1.0f, a2de::MathUtils::GetRandomFloatInRange(0.0f, 359.0f));
        }
    }
    case Type::Big:
    {
        return GetPosition() + a2de::Vector2::CreateFromPolarCoordinatesDegrees(1.0f, a2de::MathUtils::GetRandomFloatInRange(0.0f, 359.0f));
    }
    case Type::Boss:
    {
        if(const auto ship = g_theGame->GetShip()) {
            const auto target = ship->GetPosition();
            const auto source = GetPosition();
            const auto angle = (target - source).CalcHeadingDegrees();
            const auto offset_range = 90.0f;
            const auto offset = a2de::MathUtils::GetRandomFloatNegOneToOne() * offset_range;
            return GetPosition() + a2de::Vector2::CreateFromPolarCoordinatesDegrees(1.0f, angle + offset);
        } else {
            return GetPosition() + a2de::Vector2::CreateFromPolarCoordinatesDegrees(1.0f, a2de::MathUtils::GetRandomFloatInRange(0.0f, 359.0f));
        }
    }
    default: return GetPosition() + a2de::Vector2::CreateFromPolarCoordinatesDegrees(1.0f, a2de::MathUtils::GetRandomFloatInRange(0.0f, 359.0f));
    }
}

float Ufo::GetUfoIndexFromStyle(Style style) noexcept {
    switch(style) {
    case Style::Blue: return 0.0f;
    case Style::Green: return 1.0f;
    case Style::Yellow: return 2.0f;
    //case Style::Cyan:
    //    break;
    //case Style::Magenta:
    //    break;
    //case Style::Last_Big:
    //    break;
    //case Style::First_Boss:
    //    break;
    //case Style::Orange:
    //    break;
    //case Style::Last_Boss:
    //    break;
    default: return 0.0f;
    }
}

int Ufo::GetHealthFromType(Type type) noexcept {
    switch(type) {
    case Type::Small: return 1;
    case Type::Big: return 2;
    case Type::Boss: return 3;
    default: return 1;
    }
}

int Ufo::GetStartIndexFromTypeAndStyle(Type type, Style style) noexcept {
    switch(type) {
    case Type::Small:
    {
        switch(style) {
        case Style::Blue: return 0;
        case Style::Green: return 4;
        case Style::Yellow: return 8;
        }
    }
    case Type::Big:
    {
        switch(style) {
        case Style::Cyan: return 0;
        case Style::Magenta: return 4;
        default: return 4;
        }
    }
    case Type::Boss:
    {
        switch(style) {
        case Style::Orange: return 0;
        default: return 0;
        }
    }
    default: return 0;
    }
}

int Ufo::GetFrameLengthFromTypeAndStyle(Type type, Style style) noexcept {
    switch(type) {
    case Type::Small:
    {
        switch(style) {
        case Style::Blue: return 4;
        case Style::Green: return 4;
        case Style::Yellow: return 4;
        default: return 4;
        }
    }
    case Type::Big:
    {
        switch(style) {
        case Style::Cyan: return 4;
        case Style::Magenta: return 4;
        default: return 4;
        }
    }
    case Type::Boss:
    {
        switch(style) {
        case Style::Orange: return 8;
        default: return 8;
        }
    }
    default: return 4;
    }
}

long long Ufo::GetValueFromType(Type type) noexcept {
    switch(type) {
    case Type::Small: return 1000LL;
    case Type::Big: return 200LL;
    case Type::Boss: return 2000LL;
    default: return 0LL;
    }
}

Ufo::Style Ufo::GetStyleFromType(Type type) noexcept {
    switch(type) {
    case Type::Small:
    {
        const auto i = a2de::MathUtils::GetRandomIntInRange(static_cast<int>(Style::First_Small), static_cast<int>(Style::Last_Small));
        return static_cast<Style>(i);
    }
    case Type::Big:
    {
        const auto i = a2de::MathUtils::GetRandomIntInRange(static_cast<int>(Style::First_Big), static_cast<int>(Style::Last_Big));
        return static_cast<Style>(i);
    }
    case Type::Boss:
    {
        const auto i = a2de::MathUtils::GetRandomIntInRange(static_cast<int>(Style::First_Boss), static_cast<int>(Style::Last_Boss));
        return static_cast<Style>(i);
    }
    default:
    {
        return Style::Blue;
    }
    }
}


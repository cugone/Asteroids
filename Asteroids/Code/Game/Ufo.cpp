#include "Game/Ufo.hpp"

#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Shader.hpp"

#include "Game/Bullet.hpp"
#include "Game/Game.hpp"
#include "Game/Ship.hpp"

#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"

Ufo::Ufo(Type type, Vector2 position)
    : Entity()
    , _type(type)
{
    SetOrientationDegrees(-90.0f);
    SetCosmeticRadius(GetCosmeticRadiusFromType(_type));
    SetPhysicalRadius(GetPhysicalRadiusFromType(_type));
    SetPosition(position);
    SetVelocity(Vector2::X_AXIS * 100.0f);
    faction = Entity::Faction::Enemy;
    _bulletSpeed = GetBulletSpeedFromTypeAndDifficulty(_type);
    _fireRate.SetFrequency(GetFireRateFromTypeAndDifficulty(_type));
    _style = GetStyleFromType(_type);
    scoreValue = GetValueFromType(_type);

    AnimatedSpriteDesc desc{};
    desc.material = g_theRenderer->GetMaterial("ufo");
    desc.spriteSheet = GetSpriteSheetFromType(_type);
    desc.durationSeconds = TimeUtils::FPSeconds{0.3f};
    desc.playbackMode = AnimatedSprite::SpriteAnimMode::Looping;
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

void Ufo::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    Entity::Update(deltaSeconds);
    _sprite->Update(deltaSeconds);

    if(_canFire) {
        OnFire();
    }

    const auto uvs = _sprite->GetCurrentTexCoords();
    const auto frameWidth = static_cast<float>(_sprite->GetFrameDimensions().x);
    const auto frameHeight = static_cast<float>(_sprite->GetFrameDimensions().y);
    const auto half_extents = Vector2{frameWidth, frameHeight};
    {
        const auto S = Matrix4::CreateScaleMatrix(half_extents);
        const auto R = Matrix4::Create2DRotationDegreesMatrix(90.0f + GetOrientationDegrees());
        const auto T = Matrix4::CreateTranslationMatrix(GetPosition());
        transform = Matrix4::MakeSRT(S, R, T);
    }

    auto& builder = mesh_builder;
    builder.Begin(PrimitiveType::Triangles);
    builder.SetColor(Rgba::White);

    builder.SetUV(Vector2{uvs.maxs.x, uvs.maxs.y});
    builder.AddVertex(Vector2{+0.5f, +0.5f});

    builder.SetUV(Vector2{uvs.mins.x, uvs.maxs.y});
    builder.AddVertex(Vector2{-0.5f, +0.5f});

    builder.SetUV(Vector2{uvs.mins.x, uvs.mins.y});
    builder.AddVertex(Vector2{-0.5f, -0.5f});

    builder.SetUV(Vector2{uvs.maxs.x, uvs.mins.y});
    builder.AddVertex(Vector2{+0.5f, -0.5f});

    builder.AddIndicies(Mesh::Builder::Primitive::Quad);
    builder.End(material);

}

void Ufo::Render(Renderer& renderer) const noexcept {
    Entity::Render(renderer);
}

void Ufo::EndFrame() noexcept {
    Entity::EndFrame();
}

void Ufo::OnCreate() noexcept {
    AudioSystem::SoundDesc desc{};
    desc.volume = 1.0f;
    desc.frequency = 1.0f;
    desc.loopCount = -1;
    _warble_sound = g_theAudioSystem->CreateSound(g_sound_warblepath);
    g_theAudioSystem->Play(*_warble_sound, desc);
}

void Ufo::OnCollision(Entity* a, Entity* b) noexcept {
    if(auto* asBullet = dynamic_cast<Bullet*>(b); asBullet != nullptr) {
        if(b->faction != a->faction) {
            a->DecrementHealth();
            if(a->IsDead()) {
                a->Kill();
            }
        }
    }
}

void Ufo::OnFire() noexcept {
    if(_canFire) {
        _canFire = false;
        MakeBullet();
    }
}

void Ufo::OnDestroy() noexcept {
    _warble_sound->Stop();
    g_theGame->MakeExplosion(GetPosition());
}

Vector4 Ufo::WasHit() const noexcept {
    return _timeSinceLastHit.count() == 0.0f ? Vector4::X_AXIS : Vector4::ZERO;
}

void Ufo::MakeBullet() const noexcept {
    const auto source = GetPosition();
    const auto angle = (_fireTarget - source).CalcHeadingDegrees();
    g_theGame->MakeBullet(this, source, Vector2::CreateFromPolarCoordinatesDegrees(_bulletSpeed, angle));
}

float Ufo::GetCosmeticRadiusFromType(Type type) const noexcept {
    switch(type) {
    case Type::Small: return 10.0f;
    case Type::Big: return 10.0f;
    case Type::Boss: return 10.0f;
    default: return 0.0f;
    }
}

float Ufo::GetPhysicalRadiusFromType(Type type) const noexcept {
    const auto cr = GetCosmeticRadiusFromType(type);
    switch(type) {
    case Type::Small: return cr * 0.98f;
    case Type::Big: return  cr * 0.98f;
    case Type::Boss: return  cr * 0.98f;
    default: return cr;
    }
}

std::weak_ptr<SpriteSheet> Ufo::GetSpriteSheetFromType(Type type) const noexcept {
    switch(type) {
    case Type::Small: return g_theGame->ufo_small_sheet;
    case Type::Big: return g_theGame->ufo_big_sheet;
    case Type::Boss: return g_theGame->ufo_boss_sheet;
    default: return g_theGame->ufo_small_sheet;
    }

}

unsigned int Ufo::GetFireRateFromTypeAndDifficulty(Type type) const noexcept {
    switch(type) {
    case Type::Small: return 2u;
    case Type::Big: return 1u;
    case Type::Boss: return 3u;
    default: return 0u;
    }
}

float Ufo::GetBulletSpeedFromTypeAndDifficulty(Type type) const noexcept {
    const auto typeMultiplier = [this, type]()->float {
        switch(type) {
        case Type::Small: return 2.0f;
        case Type::Big: return 1.0f;
        case Type::Boss: return 3.0f;
        default: return 1.0f;
        }
    }();
    const auto difficultyMultiplier = [this]() -> float{
        switch(g_theGame->gameOptions.difficulty) {
        case Difficulty::Easy: return 0.50f;
        case Difficulty::Normal: return 1.0f;
        case Difficulty::Hard: return 2.0f;
        default: return 1.0f;
        };
    }();
    return difficultyMultiplier * typeMultiplier * _bulletSpeed;
}

Vector2 Ufo::CalculateFireTarget() const noexcept {
    switch(_type) {
    case Type::Small: {
        if(auto ship = g_theGame->GetShip()) {
            return ship->GetPosition();
        } else {
            return Vector2::CreateFromPolarCoordinatesDegrees(1.0f, MathUtils::GetRandomFloatInRange(0.0f, 359.0f));
        }
    }
    case Type::Big:
    {
        if(auto ship = g_theGame->GetShip()) {
            const auto target = ship->GetPosition();
            const auto source = GetPosition();
            const auto angle = (target - source).CalcHeadingDegrees();
            const auto offset_range = 180.0f;
            const auto offset = MathUtils::GetRandomFloatNegOneToOne() * offset_range;
            return Vector2::CreateFromPolarCoordinatesDegrees(1.0f, angle + offset);
        } else {
            return Vector2::CreateFromPolarCoordinatesDegrees(1.0f, MathUtils::GetRandomFloatInRange(0.0f, 359.0f));
        }
    }
    case Type::Boss:
    {
        if(const auto ship = g_theGame->GetShip()) {
            const auto target = ship->GetPosition();
            const auto source = GetPosition();
            const auto angle = (target - source).CalcHeadingDegrees();
            const auto offset_range = 15.0f;
            const auto offset = MathUtils::GetRandomFloatNegOneToOne() * offset_range;
            return Vector2::CreateFromPolarCoordinatesDegrees(1.0f, angle + offset);
        } else {
            return Vector2::CreateFromPolarCoordinatesDegrees(1.0f, MathUtils::GetRandomFloatInRange(0.0f, 359.0f));
        }
    }
    default: return Vector2::CreateFromPolarCoordinatesDegrees(1.0f, MathUtils::GetRandomFloatInRange(0.0f, 359.0f));
    }
}

int Ufo::GetStartIndexFromTypeAndStyle(Type type, Style style) const noexcept {
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

int Ufo::GetFrameLengthFromTypeAndStyle(Type type, Style style) const noexcept {
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

long long Ufo::GetValueFromType(Type type) const noexcept {
    switch(type) {
    case Type::Small: return 1000LL;
    case Type::Big: return 200LL;
    case Type::Boss: return 2000LL;
    default: return 0LL;
    }
}

Ufo::Style Ufo::GetStyleFromType(Type type) const noexcept {
    switch(type) {
    case Type::Small:
    {
        const auto i = MathUtils::GetRandomIntInRange(static_cast<int>(Style::First_Small), static_cast<int>(Style::Last_Small));
        return static_cast<Style>(i);
    }
    case Type::Big:
    {
        const auto i = MathUtils::GetRandomIntInRange(static_cast<int>(Style::First_Big), static_cast<int>(Style::Last_Big));
        return static_cast<Style>(i);
    }
    case Type::Boss:
    {
        const auto i = MathUtils::GetRandomIntInRange(static_cast<int>(Style::First_Boss), static_cast<int>(Style::Last_Boss));
        return static_cast<Style>(i);
    }
    default:
    {
        return Style::Blue;
    }
    }
}


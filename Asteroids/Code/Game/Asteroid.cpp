#include "Game/Asteroid.hpp"

#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/MathUtils.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"

#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"
#include "Game/Game.hpp"

#include "Game/Bullet.hpp"
#include "Game/Mine.hpp"

#include <algorithm>
#include <type_traits>
#include <vector>

Asteroid::Asteroid(Vector2 position, Vector2 velocity, float rotationSpeed)
    : Asteroid(Type::Large, position, velocity, rotationSpeed) {/* DO NOTHING */}

Asteroid::Asteroid(Type type, Vector2 position, Vector2 velocity, float rotationSpeed)
    : Entity()
    , _type(type)
{
    faction = Entity::Faction::Asteroid;
    scoreValue = GetScoreFromType(type);
    SetHealth(GetHealthFromType(type));
    SetPosition(position);
    SetVelocity(velocity);
    SetRotationSpeed(rotationSpeed);
    auto [cosmeticRadius, physicalRadius] = GetRadiiFromType(type);
    SetCosmeticRadius(cosmeticRadius);
    SetPhysicalRadius(physicalRadius);

    AnimatedSpriteDesc desc{};
    desc.material = g_theRenderer->GetMaterial("asteroid");
    desc.spriteSheet = g_theGame->asteroid_sheet;
    desc.durationSeconds = TimeUtils::FPSeconds{1.0f};
    desc.playbackMode = AnimatedSprite::SpriteAnimMode::Looping;
    desc.frameLength = 30;
    desc.startSpriteIndex = 0;
    
    material = desc.material;

    if(auto cbs = material->GetShader()->GetConstantBuffers(); !cbs.empty()) {
        asteroid_state_cb = &cbs[0].get();
    }

    _sprite = g_theRenderer->CreateAnimatedSprite(desc);

}

long long Asteroid::GetScoreFromType(Type type) {
    switch(type) {
    case Type::Large: return 25LL;
    case Type::Medium: return 50LL;
    case Type::Small: return 100LL;
    default: return 0LL;
    }
}

void Asteroid::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    Entity::Update(deltaSeconds);
    _timeSinceLastHit += deltaSeconds;

    const auto theta = GetRotationSpeed() * deltaSeconds.count();
    if(theta < 0.0f) {
        RotateClockwise(theta);
    } else if(theta > 0.0f) {
        RotateCounterClockwise(theta);
    };
    _sprite->Update(deltaSeconds);

    const auto uvs = _sprite->GetCurrentTexCoords();
    const auto frameWidth = static_cast<float>(_sprite->GetFrameDimensions().x);
    const auto frameHeight = static_cast<float>(_sprite->GetFrameDimensions().y);
    const auto extent_scale = _type == Type::Large ? 1.0f : (_type == Type::Medium ? 0.75f : (_type == Type::Small ? 0.50f : 1.0f));
    const auto half_extents = Vector2{frameWidth, frameHeight} * extent_scale;
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

void Asteroid::Render(Renderer& renderer) const noexcept {
    asteroid_state.wasHit = WasHit();
    asteroid_state_cb->Update(*renderer.GetDeviceContext(), &asteroid_state);
    Entity::Render(renderer);
}

Vector4 Asteroid::WasHit() const noexcept {
    return _timeSinceLastHit.count() == 0.0f ? Vector4::X_AXIS : Vector4::ZERO;
}


void Asteroid::EndFrame() noexcept {
    Entity::EndFrame();
    if(const auto& found = std::find(std::begin(g_theGame->asteroids), std::end(g_theGame->asteroids), this);
        (found != std::end(g_theGame->asteroids) &&
    (*found)->IsDead()))
    {
        *found = nullptr;
    }
}

void Asteroid::OnDestroy() noexcept {
    Entity::OnDestroy();
    switch(_type) {
    case Type::Large:
    {
        MakeChildAsteroid();
        MakeChildAsteroid();
        break;
    }
    case Type::Medium:
    {
        MakeChildAsteroid();
        MakeChildAsteroid();
        MakeChildAsteroid();
        MakeChildAsteroid();
        break;
    }
    case Type::Small:
    {
        /* DO NOTHING */
        break;
    }
    default:
    {
        /* DO NOTHING */
        break;
    }
    }
}

void Asteroid::MakeChildAsteroid() const noexcept {
    const auto [position, velocity, rotation] = CalcChildPhysicsParameters();
    switch(_type) {
    case Type::Large:
        g_theGame->MakeMediumAsteroid(position, velocity, rotation);
        break;
    case Type::Medium:
        g_theGame->MakeSmallAsteroid(position, velocity, rotation);
        break;
    case Type::Small:
        break;
    default:
        break;
    }
}

void Asteroid::OnFire() noexcept {
    /* DO NOTHING */
}

void Asteroid::OnHit() noexcept {
    if(TimeUtils::FPFrames{1.0f} < _timeSinceLastHit) {
        _timeSinceLastHit = _timeSinceLastHit.zero();
    }
    g_theAudioSystem->Play(g_sound_hitpath);
    asteroid_state.wasHit = WasHit();
}

void Asteroid::OnCollision(Entity* a, Entity* b) noexcept {
    if(a->faction == b->faction) {
        return;
    }
    switch(b->faction) {
    case Entity::Faction::Player:
    {
        if(auto* asBullet = dynamic_cast<Bullet*>(b); asBullet != nullptr) {
            a->DecrementHealth();
            asBullet->DecrementHealth();
            OnHit();
        }
        if(auto* asMine = dynamic_cast<Mine*>(b); asMine != nullptr) {
            a->Kill();
            b->Kill();
            OnHit();
        }
    }
    break;
    case Entity::Faction::Enemy:
    /* DO NOTHING */
    break;
    default: break;
    }
    

}

void Asteroid::OnCreate() noexcept {
    /* DO NOTHING */
}

std::pair<float, float> Asteroid::GetRadiiFromType(Type type) const noexcept {
    switch(type) {
    case Type::Large:
        return std::make_pair(50.0f, 40.0f);
    case Type::Medium:
        return std::make_pair(25.0f, 20.0f);
    case Type::Small:
        return std::make_pair(12.0f, 10.0f);
    default:
        return std::make_pair(50.0f, 40.0f);
    }
}

int Asteroid::GetHealthFromType(Type type) const noexcept {
    switch(type) {
    case Type::Large:
        return 3;
    case Type::Medium:
        return 2;
    case Type::Small:
        return 1;
    default:
        return 1;
    }
}

float Asteroid::CalcChildHeadingFromDifficulty() const noexcept {
    const auto currentHeading = GetVelocity().CalcHeadingDegrees();
    switch(g_theGame->gameOptions.difficulty) {
    case Difficulty::Easy:
        return MathUtils::GetRandomFloatZeroToOne() * 360.0f;
    case Difficulty::Normal:
        return currentHeading + MathUtils::GetRandomFloatNegOneToOne() * 90.0f;
    case Difficulty::Hard:
        return currentHeading + MathUtils::GetRandomFloatNegOneToOne() * 45.0f;
    default:
        return currentHeading;
    }
}

float Asteroid::CalcChildSpeedFromSizeAndDifficulty() const noexcept{
    const auto currentSpeed = CalcChildSpeedFromSize();
    switch(g_theGame->gameOptions.difficulty) {
    case Difficulty::Easy:
        return currentSpeed * 0.5f;
    case Difficulty::Normal:
        return currentSpeed * 1.0f;
    case Difficulty::Hard:
        return currentSpeed * 1.5f;
    default:
        return currentSpeed;
    }
}

float Asteroid::CalcChildSpeedFromSize() const noexcept {
    const auto currentSpeed = GetVelocity().CalcLength();
    switch(_type) {
    case Type::Large:
        return currentSpeed * MathUtils::GetRandomFloatInRange(2.0f, 2.2f);
    case Type::Medium:
        return currentSpeed * MathUtils::GetRandomFloatInRange(2.5f, 2.6f);
    case Type::Small:
        return currentSpeed;
    default:
        return currentSpeed;
    }
}

std::tuple<Vector2, Vector2, float> Asteroid::CalcChildPhysicsParameters() const noexcept {
    const auto heading = CalcChildHeadingFromDifficulty();
    const auto speed = CalcChildSpeedFromSizeAndDifficulty();
    auto v = GetVelocity();
    v.SetLengthAndHeadingDegrees(heading, speed);
    const auto r = MathUtils::GetRandomFloatZeroToOne() * 360.0f;
    const auto p = MathUtils::GetRandomPointInside(Disc2{GetPosition(), GetCosmeticRadius()});
    return std::make_tuple(p,v,r);
}

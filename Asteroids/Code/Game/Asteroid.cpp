#include "Game/Asteroid.hpp"

#include "Engine/Math/Disc2.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"

#include "Game/GameCommon.hpp"

#include "Game/Bullet.hpp"

#include <algorithm>
#include <type_traits>
#include <vector>

Asteroid::Asteroid(Vector2 position, Vector2 velocity, float rotationSpeed)
    : Asteroid(Type::Large, position, velocity, rotationSpeed) {/* DO NOTHING */}

Asteroid::Asteroid(Type type, Vector2 position, Vector2 velocity, float rotationSpeed)
    : Entity()
    , _type(type)
{
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

    asteroid_state.wasHit = WasHit();
    asteroid_state_cb->Update(*g_theRenderer->GetDeviceContext(), &asteroid_state);
}

Vector4 Asteroid::WasHit() noexcept {
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
        const auto heading1 = MathUtils::GetRandomFloatZeroToOne() * 360.0f;
        const auto heading2 = MathUtils::GetRandomFloatZeroToOne() * 360.0f;
        auto v1 = GetVelocity() * MathUtils::GetRandomFloatInRange(1.5f, 5.0f);
        v1.SetHeadingDegrees(heading1);
        auto v2 = GetVelocity() * MathUtils::GetRandomFloatInRange(1.5f, 5.0f);
        v2.SetHeadingDegrees(heading2);
        const auto rotSpeed1 = MathUtils::GetRandomFloatZeroToOne() * 360.0f;
        const auto rotSpeed2 = MathUtils::GetRandomFloatZeroToOne() * 360.0f;
        const auto p1 = MathUtils::GetRandomPointInside(Disc2{GetPosition(), GetCosmeticRadius()});
        const auto p2 = MathUtils::GetRandomPointInside(Disc2{GetPosition(), GetCosmeticRadius()});
        g_theGame->MakeMediumAsteroid(p1, v1, rotSpeed1);
        g_theGame->MakeMediumAsteroid(p2, v2, rotSpeed2);
        break;
    }
    case Type::Medium:
    {
        const auto heading1 = MathUtils::GetRandomFloatZeroToOne() * 360.0f;
        const auto heading2 = MathUtils::GetRandomFloatZeroToOne() * 360.0f;
        const auto heading3 = MathUtils::GetRandomFloatZeroToOne() * 360.0f;
        const auto heading4 = MathUtils::GetRandomFloatZeroToOne() * 360.0f;
        auto v1 = GetVelocity() * MathUtils::GetRandomFloatInRange(2.5f, 5.0f);
        v1.SetHeadingDegrees(heading1);
        auto v2 = GetVelocity() * MathUtils::GetRandomFloatInRange(2.5f, 5.0f);
        v2.SetHeadingDegrees(heading2);
        auto v3 = GetVelocity() * MathUtils::GetRandomFloatInRange(2.5f, 5.0f);
        v3.SetHeadingDegrees(heading3);
        auto v4 = GetVelocity() * MathUtils::GetRandomFloatInRange(2.5f, 5.0f);
        v4.SetHeadingDegrees(heading4);
        const auto rotSpeed1 = MathUtils::GetRandomFloatZeroToOne() * 360.0f;
        const auto rotSpeed2 = MathUtils::GetRandomFloatZeroToOne() * 360.0f;
        const auto rotSpeed3 = MathUtils::GetRandomFloatZeroToOne() * 360.0f;
        const auto rotSpeed4 = MathUtils::GetRandomFloatZeroToOne() * 360.0f;
        const auto p1 = MathUtils::GetRandomPointInside(Disc2{GetPosition(), GetCosmeticRadius()});
        const auto p2 = MathUtils::GetRandomPointInside(Disc2{GetPosition(), GetCosmeticRadius()});
        const auto p3 = MathUtils::GetRandomPointInside(Disc2{GetPosition(), GetCosmeticRadius()});
        const auto p4 = MathUtils::GetRandomPointInside(Disc2{GetPosition(), GetCosmeticRadius()});
        g_theGame->MakeSmallAsteroid(p1, v1, rotSpeed1);
        g_theGame->MakeSmallAsteroid(p2, v2, rotSpeed2);
        g_theGame->MakeSmallAsteroid(p3, v3, rotSpeed3);
        g_theGame->MakeSmallAsteroid(p4, v4, rotSpeed4);
        break;
    }
    case Type::Small:
        /* DO NOTHING */
        break;
    default: /* DO NOTHING */;
    }
}

void Asteroid::OnFire() noexcept {
    /* DO NOTHING */
}

void Asteroid::OnCollision(Entity* a, Entity* b) noexcept {
    const auto* asBullet = dynamic_cast<Bullet*>(b);
    if(asBullet) {
        if(TimeUtils::FPFrames{1.0f} < _timeSinceLastHit) {
            _timeSinceLastHit = _timeSinceLastHit.zero();
        }
        a->DecrementHealth();
        g_theAudioSystem->Play("Data/Audio/Hit.wav");
        //asteroid_state.wasHit = WasHit();
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

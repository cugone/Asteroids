#include "Game/Asteroid.hpp"

#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Core/EngineCommon.hpp"

#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/MathUtils.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"
#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"

#include "Engine/Scene/Components.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"
#include "Game/Game.hpp"
#include "Game/MainState.hpp"

#include "Game/Bullet.hpp"
#include "Game/Mine.hpp"

#include <algorithm>
#include <type_traits>
#include <vector>

Asteroid::Asteroid(std::weak_ptr<Scene> scene, Vector2 position, Vector2 velocity, float rotationSpeed)
    : Asteroid(scene, Type::Large, position, velocity, rotationSpeed) {/* DO NOTHING */}

Asteroid::Asteroid(std::weak_ptr<Scene> scene, Type type, Vector2 position, Vector2 velocity, float rotationSpeed)
    : GameEntity(scene.lock()->CreateEntity(), scene)
    , _type(type)
{
    UpdateComponent<TransformComponent>(Matrix4::CreateTranslationMatrix(position));
    faction = GameEntity::Faction::Asteroid;
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
    desc.spriteSheet = GetGameAs<Game>()->asteroid_sheet;
    desc.durationSeconds = TimeUtils::FPSeconds{1.0f};
    desc.playbackMode = AnimatedSprite::SpriteAnimMode::Looping;
    desc.frameLength = 30;
    desc.startSpriteIndex = 0;

    if(auto cbs = GetMaterial()->GetShader()->GetConstantBuffers(); !cbs.empty()) {
        asteroid_state_cb = &cbs[0].get();
    }

    _sprite = g_theRenderer->CreateAnimatedSprite(desc);

}

void Asteroid::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    GameEntity::Update(deltaSeconds);
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
        const auto R = Matrix4::Create2DRotationDegreesMatrix(GetOrientationDegrees());
        const auto T = Matrix4::CreateTranslationMatrix(GetPosition());
        UpdateComponent<TransformComponent>(Matrix4::MakeSRT(S, R, T));
    }

    auto& builder = m_mesh_builder;
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
    builder.End(GetMaterial());

}

void Asteroid::Render() const noexcept {
    asteroid_state.wasHit = WasHit();
    asteroid_state_cb->Update(*ServiceLocator::get<IRendererService>().GetDeviceContext(), &asteroid_state);
    GameEntity::Render();
}

Vector4 Asteroid::WasHit() const noexcept {
    return _timeSinceLastHit.count() == 0.0f ? Vector4::X_Axis : Vector4::Zero;
}


void Asteroid::EndFrame() noexcept {
    GameEntity::EndFrame();
}

void Asteroid::OnDestroy() noexcept {
    GameEntity::OnDestroy();
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

Material* Asteroid::GetMaterial() const noexcept {
    return g_theRenderer->GetMaterial("asteroid");
}

void Asteroid::MakeChildAsteroid() const noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        if (auto* const mainState = dynamic_cast<MainState* const>(game->GetCurrentState()); mainState != nullptr) {
            const auto [position, velocity, rotation] = CalcChildPhysicsParameters();
            switch (_type) {
            case Type::Large:
                mainState->MakeMediumAsteroid(position, velocity, rotation);
                break;
            case Type::Medium:
                mainState->MakeSmallAsteroid(position, velocity, rotation);
                break;
            case Type::Small:
                break;
            default:
                break;
            }
        }
    }
}

void Asteroid::OnFire() noexcept {
    /* DO NOTHING */
}

void Asteroid::OnHit() noexcept {
    if(TimeUtils::FPFrames{1.0f} < _timeSinceLastHit) {
        _timeSinceLastHit = _timeSinceLastHit.zero();
    }
    AudioSystem::SoundDesc desc{};
    desc.groupName = g_audiogroup_sound;
    g_theAudioSystem->Play(g_sound_hitpath, desc);
    asteroid_state.wasHit = WasHit();
}

void Asteroid::OnCollision(GameEntity* a, GameEntity* b) noexcept {
    if(a->faction == b->faction) {
        return;
    }
    switch(b->faction) {
    case GameEntity::Faction::Player:
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
    case GameEntity::Faction::Enemy:
    /* DO NOTHING */
    break;
    default: break;
    }
    

}

void Asteroid::OnCreate() noexcept {
    /* DO NOTHING */
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
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        switch(game->gameOptions.GetDifficulty()) {
        case Difficulty::Easy:
            return MathUtils::GetRandomZeroToOne<float>() * 360.0f;
        case Difficulty::Normal:
            return currentHeading + MathUtils::GetRandomNegOneToOne<float>() * 90.0f;
        case Difficulty::Hard:
            return currentHeading + MathUtils::GetRandomNegOneToOne<float>() * 45.0f;
        default:
            return currentHeading;
        }
    }
    return currentHeading;
}

float Asteroid::CalcChildSpeedFromSizeAndDifficulty() const noexcept{
    const auto currentSpeed = CalcChildSpeedFromSize();
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        switch(game->gameOptions.GetDifficulty()) {
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
    return currentSpeed;
}

float Asteroid::CalcChildSpeedFromSize() const noexcept {
    const auto currentSpeed = GetVelocity().CalcLength();
    switch(_type) {
    case Type::Large:
        return currentSpeed * MathUtils::GetRandomInRange<float>(2.0f, 2.2f);
    case Type::Medium:
        return currentSpeed * MathUtils::GetRandomInRange<float>(2.5f, 2.6f);
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
    const auto r = MathUtils::GetRandomZeroToOne<float>() * 360.0f;
    const auto p = MathUtils::GetRandomPointInside(Disc2{GetPosition(), GetCosmeticRadius()});
    return std::make_tuple(p,v,r);
}

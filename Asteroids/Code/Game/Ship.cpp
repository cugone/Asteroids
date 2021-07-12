#include "Game/Ship.hpp"

#include "Engine/Renderer/Vertex3D.hpp"
#include "Engine/Core/Rgba.hpp"

#include "Engine/Math/Vector3.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"

#include "Game/Asteroid.hpp"
#include "Game/Bullet.hpp"
#include "Game/Ufo.hpp"
#include "Game/Weapon.hpp"

#include "Game/ThrustComponent.hpp"

#include <algorithm>

Ship::Ship() : Ship(Vector2::ZERO) {}

Ship::Ship(Vector2 position)
    : Entity()
{
    faction = Entity::Faction::Player;
    _thrust = std::move(std::make_unique<ThrustComponent>(this));
    material = g_theRenderer->GetMaterial("ship");
    scoreValue = -100LL;
    SetPosition(position);
    SetOrientationDegrees(-90.0f);
    SetCosmeticRadius(25.0f);
    SetPhysicalRadius(15.0f);
    _fireRate.SetFrequency(10u);
    _mineFireRate.SetFrequency(1u);
}

void Ship::BeginFrame() noexcept {
    Entity::BeginFrame();
    _thrust->BeginFrame();
}

void Ship::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    Entity::Update(deltaSeconds);
    if(GetForce().CalcLengthSquared() == 0.0f) {
        auto newVelocity = GetVelocity() * 0.99f;
        if(MathUtils::IsEquivalentToZero(newVelocity)) {
            newVelocity = Vector2::ZERO;
        }
        SetVelocity(newVelocity);
    }
    const auto uvs = AABB2::ZERO_TO_ONE;
    const auto tex = material->GetTexture(Material::TextureID::Diffuse);
    const auto frameWidth = static_cast<float>(tex->GetDimensions().x);
    const auto frameHeight = static_cast<float>(tex->GetDimensions().y);
    const auto half_extents = Vector2{frameWidth, frameHeight};

    DoScaleEaseOut(deltaSeconds);

    const auto S = Matrix4::CreateScaleMatrix(_scale * half_extents);
    const auto R = Matrix4::Create2DRotationDegreesMatrix(GetOrientationDegrees());
    const auto T = Matrix4::CreateTranslationMatrix(GetPosition());
    transform = Matrix4::MakeSRT(S, R, T);
    
    auto& builder = mesh_builder;
    builder.Begin(PrimitiveType::Triangles);
    if(IsRespawning()) {
        builder.SetAlpha(DoAlphaEaseOut(deltaSeconds));
    } else {
        builder.SetColor(Rgba::White);
    }

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

    if(!IsRespawning()) {
        _thrust->Update(deltaSeconds);
    }

}

void Ship::Render() const noexcept {
    _thrust->Render();
    Entity::Render();
}

void Ship::DoScaleEaseOut(TimeUtils::FPSeconds& deltaSeconds) noexcept {
    static float t = 0.0f;
    static float duration = 0.66f;
    static float startScale = 4.0f;
    static float endScale = 1.0f;
    if(IsRespawning()) {
        if(t < duration) {
            _scale = MathUtils::Interpolate(endScale, startScale, MathUtils::EasingFunctions::SmoothStop<3>(t / duration));
            t += deltaSeconds.count();
        } else {
            t = 0.0f;
            _scale = 1.0f;
            DoneRespawning();
        }
    } else {
        _scale = 1.0f;
    }
}


float Ship::DoAlphaEaseOut(TimeUtils::FPSeconds& deltaSeconds) const noexcept {
    static float t = 0.0f;
    static float duration = 0.66f;
    static float start = 0.0f;
    static float end = 1.0f;
    static float a = 0.0f;
    if(t < duration) {
        a = MathUtils::Interpolate(end, start, MathUtils::EasingFunctions::SmoothStop<3>(t / duration));
        t += deltaSeconds.count();
    } else {
        t = 0.0f;
        a = 0.0f;
    }
    return a;
}

void Ship::EndFrame() noexcept {
    Entity::EndFrame();
    if(!IsRespawning() && _fireRate.CheckAndReset()) {
        _canFire = true;
    }
    if(!IsRespawning() && _mineFireRate.CheckAndReset()) {
        _canDropMine = true;
    }
    _thrust->EndFrame();
}

void Ship::OnDestroy() noexcept {
    if(IsRespawning()) {
        return;
    }
    Entity::OnDestroy();
    g_theGame->MakeExplosion(GetPosition());
    SetRespawning();
    g_theGame->respawnTimer.Reset();
}

void Ship::OnFire() noexcept {
    if(IsRespawning()) {
        return;
    }
    if(_canFire) {
        _canFire = false;
        MakeBullet();
    }
}

void Ship::DropMine() noexcept {
    if(IsRespawning()) {
        return;
    }
    if(_canDropMine) {
        _canDropMine = false;
        MakeMine();
    }
}

void Ship::Thrust(float force) noexcept {
    if(IsRespawning()) {
        return;
    }
    _thrust->SetThrust(force);
    AddForce(GetForward() * force);
}

void Ship::SetRespawning() noexcept {
    _respawning = true;
}

const bool Ship::IsRespawning() const noexcept {
    return _respawning;
}

void Ship::DoneRespawning() noexcept {
    _respawning = false;
}

void Ship::OnCollision(Entity* a, Entity* b) noexcept {
    if(IsRespawning()) {
        return;
    }
    if(a->faction == b->faction) {
        return;
    }
    switch(b->faction) {
    case Entity::Faction::Enemy:
    {
        if(auto* asBullet = dynamic_cast<Bullet*>(b); asBullet != nullptr) {
            a->DecrementHealth();
            asBullet->DecrementHealth();
            if(a->IsDead()) {
                g_theGame->DecrementLives();
            }
        } else if(auto* asUfo = dynamic_cast<Ufo*>(b); asUfo != nullptr) {
            a->Kill();
            g_theGame->DecrementLives();
        }
    }
    break;
    case Entity::Faction::Asteroid:
    {
        if(auto* asAsteroid = dynamic_cast<Asteroid*>(b); asAsteroid != nullptr) {
            a->DecrementHealth();
            if(a->IsDead()) {
                g_theGame->DecrementLives();
            }
        }
    }
    break;
    default: break;
    }
}

void Ship::OnCreate() noexcept {
    SetRespawning();
}

void Ship::MakeBullet() const noexcept {
    g_theGame->MakeBullet(this, CalcNewBulletPosition(), CalcNewBulletVelocity());
}

void Ship::MakeMine() const noexcept {
    g_theGame->MakeMine(this, GetPosition());
}

const Vector2 Ship::CalcNewBulletVelocity() const noexcept {
    return CalcBulletDirectionFromDifficulty() * _bulletSpeed;
}

const Vector2 Ship::CalcNewBulletPosition() const noexcept {
    return GetPosition() + GetForward() * GetCosmeticRadius();
}

const Vector2 Ship::CalcBulletDirectionFromDifficulty() const noexcept {
    const auto current_angle = GetForward().CalcHeadingDegrees();
    const auto angle_bias = []() {
        switch(g_theGame->gameOptions.difficulty) {
        case Difficulty::Easy: return MathUtils::GetRandomFloatNegOneToOne() * 2.5f;
        case Difficulty::Normal: return MathUtils::GetRandomFloatNegOneToOne() * 5.0f;
        case Difficulty::Hard: return MathUtils::GetRandomFloatNegOneToOne() * 10.0f;
        default: return 0.0f;
        }
    }();
    const auto new_angle = current_angle + angle_bias;
    return Vector2::CreateFromPolarCoordinatesDegrees(1.0f, new_angle);
}

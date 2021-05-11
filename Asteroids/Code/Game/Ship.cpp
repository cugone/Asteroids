#include "Game/Ship.hpp"

#include "Engine/Core/Vertex3D.hpp"
#include "Engine/Core/Rgba.hpp"

#include "Engine/Math/Vector3.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"

#include "Game/Asteroid.hpp"
#include "Game/Bullet.hpp"
#include "Game/Ufo.hpp"

#include "Game/ThrustComponent.hpp"
#include "Game/LaserComponent.hpp"
#include "Game/LaserChargeComponent.hpp"

#include <algorithm>

Ship::Ship() : Ship(a2de::Vector2::ZERO) {}

Ship::Ship(a2de::Vector2 position)
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
    _thrust->BeginFrame();
    if(_laserCharge) {
        _laserCharge->BeginFrame();
        if(_laserCharge->DoneFiring()) {
            Laser();
        }
    }
    if(_laser) {
        _laser->BeginFrame();
    }
}

void Ship::Update(a2de::TimeUtils::FPSeconds deltaSeconds) noexcept {
    Entity::Update(deltaSeconds);
    if(GetForce().CalcLengthSquared() == 0.0f) {
        auto newVelocity = GetVelocity() * 0.99f;
        if(a2de::MathUtils::IsEquivalentToZero(newVelocity)) {
            newVelocity = a2de::Vector2::ZERO;
        }
        SetVelocity(newVelocity);
    }
    const auto uvs = a2de::AABB2::ZERO_TO_ONE;
    const auto tex = material->GetTexture(a2de::Material::TextureID::Diffuse);
    const auto frameWidth = static_cast<float>(tex->GetDimensions().x);
    const auto frameHeight = static_cast<float>(tex->GetDimensions().y);
    const auto half_extents = a2de::Vector2{frameWidth, frameHeight};

    DoScaleEaseOut(deltaSeconds);

    const auto S = a2de::Matrix4::CreateScaleMatrix(_scale * half_extents);
    const auto R = a2de::Matrix4::Create2DRotationDegreesMatrix(90.0f + GetOrientationDegrees());
    const auto T = a2de::Matrix4::CreateTranslationMatrix(GetPosition());
    transform = a2de::Matrix4::MakeSRT(S, R, T);
    
    auto& builder = mesh_builder;
    builder.Begin(a2de::PrimitiveType::Triangles);
    if(IsRespawning()) {
        float a = DoAlphaEaseOut(deltaSeconds);
        builder.SetColor(a2de::Vector4{1.0f, 1.0f, 1.0f, a});
    } else {
        builder.SetColor(a2de::Rgba::White);
    }

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

    if(!IsRespawning()) {
        _thrust->Update(deltaSeconds);
        if(_laser) {
            _laser->Update(deltaSeconds);
        }
        if(_laserCharge) {
            _laserCharge->Update(deltaSeconds);
        }
    }

}

void Ship::Render(a2de::Renderer& renderer) const noexcept {
    _thrust->Render(renderer);
    if(_laser) {
        _laser->Render(renderer);
    }
    if(_laserCharge) {
        _laserCharge->Render(renderer);
    }
    Entity::Render(renderer);
}

void Ship::DoScaleEaseOut(a2de::TimeUtils::FPSeconds& deltaSeconds) noexcept {
    static float t = 0.0f;
    static float duration = 0.66f;
    static float startScale = 4.0f;
    static float endScale = 1.0f;
    if(IsRespawning()) {
        if(t < duration) {
            _scale = a2de::MathUtils::Interpolate(startScale, endScale, t / duration);
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


float Ship::DoAlphaEaseOut(a2de::TimeUtils::FPSeconds& deltaSeconds) const noexcept {
    static float t = 0.0f;
    static float duration = 0.66f;
    static float start = 0.0f;
    static float end = 1.0f;
    static float a = 0.0f;
    if(t < duration) {
        a = a2de::MathUtils::Interpolate(start, end, t / duration);
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
    if(_laser) {
        _laser->EndFrame();
        if(_laser->DoneFiring()) {
            _laser.reset();
        }
    }
    if(_laserCharge) {
        _laserCharge->EndFrame();
        if(_laserCharge->DoneFiring()) {
            _laserCharge.reset();
        }
    }
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

void Ship::Laser() noexcept {
    if(IsRespawning() || _laser) {
        return;
    }
    _laser = std::move(std::make_unique<LaserComponent>(this));
}

void Ship::Charge() noexcept {
    if(IsRespawning() || _laserCharge) {
        return;
    }
    _laserCharge = std::move(std::make_unique<LaserChargeComponent>(this));
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

const a2de::Vector2 Ship::CalcNewBulletVelocity() const noexcept {
    return CalcBulletDirectionFromDifficulty() * _bulletSpeed;
}

const a2de::Vector2 Ship::CalcNewBulletPosition() const noexcept {
    return GetPosition() + GetForward() * GetCosmeticRadius();
}

const a2de::Vector2 Ship::CalcBulletDirectionFromDifficulty() const noexcept {
    const auto current_angle = GetForward().CalcHeadingDegrees();
    const auto angle_bias = []() {
        switch(g_theGame->gameOptions.difficulty) {
        case a2de::Difficulty::Easy: return a2de::MathUtils::GetRandomFloatNegOneToOne() * 2.5f;
        case a2de::Difficulty::Normal: return a2de::MathUtils::GetRandomFloatNegOneToOne() * 5.0f;
        case a2de::Difficulty::Hard: return a2de::MathUtils::GetRandomFloatNegOneToOne() * 10.0f;
        default: return 0.0f;
        }
    }();
    const auto new_angle = current_angle + angle_bias;
    return a2de::Vector2::CreateFromPolarCoordinatesDegrees(1.0f, new_angle);
}

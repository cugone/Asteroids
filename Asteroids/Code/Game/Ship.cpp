#include "Game/Ship.hpp"

#include "Engine/Renderer/Vertex3D.hpp"
#include "Engine/Core/Rgba.hpp"
#include "Engine/Core/EngineCommon.hpp"

#include "Engine/Math/Vector3.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"

#include "Engine/Scene/Components.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"
#include "Game/MainState.hpp"

#include "Game/Asteroid.hpp"
#include "Game/Bullet.hpp"
#include "Game/Ufo.hpp"
#include "Game/IWeapon.hpp"

#include "Game/ThrustComponent.hpp"

#include <algorithm>

Ship::Ship(std::weak_ptr<Scene> scene) : Ship(scene, Vector2::Zero) {}

Ship::Ship(std::weak_ptr<Scene> scene, Vector2 position)
    : GameEntity(scene.lock()->CreateEntity(), scene)
{
    UpdateComponent<TransformComponent>(Matrix4::MakeRT(Matrix4::Create2DRotationDegreesMatrix(-90.0f), Matrix4::CreateTranslationMatrix(position)));
    faction = GameEntity::Faction::Player;
    _thrust = std::move(std::make_unique<ThrustComponent>(scene, this));

    scoreValue = -100LL;
    SetPosition(position);
    SetOrientationDegrees(-90.0f);
    SetCosmeticRadius(25.0f);
    SetPhysicalRadius(15.0f);
    
    WeaponDesc laser_weaponDesc{};
    laser_weaponDesc.bulletSpeed = 400.0f;
    laser_weaponDesc.fireRate = 0.1f;
    laser_weaponDesc.fireDelay = TimeUtils::FPSeconds{0.0f};
    laser_weaponDesc.materialName = "bullet";
    laser_weaponDesc.scale = 1.0f;

    _laserWeapon.Initialize(laser_weaponDesc);
    m_weapon = &_laserWeapon;
    _mineFireRate.SetFrequency(1u);
}

void Ship::BeginFrame() noexcept {
    GameEntity::BeginFrame();
    _thrust->BeginFrame();
}

void Ship::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    GameEntity::Update(deltaSeconds);

    const auto uvs = AABB2::Zero_to_One;
    const auto tex = GetMaterial()->GetTexture(Material::TextureID::Diffuse);
    const auto frameWidth = static_cast<float>(tex->GetDimensions().x);
    const auto frameHeight = static_cast<float>(tex->GetDimensions().y);
    const auto half_extents = Vector2{frameWidth, frameHeight};

    DoScaleEaseOut(deltaSeconds);
    {
        const auto S = Matrix4::CreateScaleMatrix(_scale * half_extents);
        const auto R = Matrix4::Create2DRotationDegreesMatrix(GetOrientationDegrees());
        const auto T = Matrix4::CreateTranslationMatrix(GetPosition());
        UpdateComponent<TransformComponent>(Matrix4::MakeSRT(S, R, T));
    }
    auto& builder = m_mesh_builder;
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
    builder.End(GetMaterial());

    if(!IsRespawning()) {
        _laserWeapon.Update(deltaSeconds);
        _thrust->Update(deltaSeconds);
    }

}

void Ship::Render() const noexcept {
    _thrust->Render();
    GameEntity::Render();
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
    GameEntity::EndFrame();
    if(!IsRespawning() && _mineFireRate.CheckAndReset()) {
        _canDropMine = true;
    }
    _thrust->EndFrame();
}

void Ship::OnDestroy() noexcept {
    if(IsRespawning()) {
        return;
    }
    GameEntity::OnDestroy();
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        if (auto* const mainState = dynamic_cast<MainState* const>(game->GetCurrentState()); mainState != nullptr) {
            mainState->MakeExplosion(GetPosition());
            SetRespawning();
            game->respawnTimer.Reset();
        }
    }
}

void Ship::OnFire() noexcept {
    if(IsRespawning()) {
        return;
    }
    if(_laserWeapon.Fire()) {
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

Material* Ship::GetMaterial() const noexcept {
    return g_theRenderer->GetMaterial("ship");
}

void Ship::Thrust(float force) noexcept {
    if(IsRespawning()) {
        return;
    }
    _thrust->SetThrust(force);
    AddForce(GetForward() * force);
}

void Ship::StopThrust() noexcept {
    _thrust->SetThrust(0.0f);
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

void Ship::OnCollision(GameEntity* a, GameEntity* b) noexcept {
    if(IsRespawning()) {
        return;
    }
    if(a->faction == b->faction) {
        return;
    }
    switch(b->faction) {
    case GameEntity::Faction::Enemy:
    {
        if(auto* game = GetGameAs<Game>(); game != nullptr) {
            if(auto* asBullet = dynamic_cast<Bullet*>(b); asBullet != nullptr) {
                a->DecrementHealth();
                asBullet->DecrementHealth();
                if(a->IsDead()) {
                    game->DecrementLives();
                }
            } else if(auto* asUfo = dynamic_cast<Ufo*>(b); asUfo != nullptr) {
                a->Kill();
                game->DecrementLives();
            }
        }
    }
    break;
    case GameEntity::Faction::Asteroid:
    {
        if(auto* game = GetGameAs<Game>(); game != nullptr) {
            if(auto* asAsteroid = dynamic_cast<Asteroid*>(b); asAsteroid != nullptr) {
                a->DecrementHealth();
                if(a->IsDead()) {
                    game->DecrementLives();
                }
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
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        if (auto* const mainState = dynamic_cast<MainState* const>(game->GetCurrentState()); mainState != nullptr) {
            mainState->MakeBullet(this, CalcNewBulletPosition(), CalcNewBulletVelocity());
        }
    }
}

void Ship::MakeMine() const noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        if (auto* const mainState = dynamic_cast<MainState* const>(game->GetCurrentState()); mainState != nullptr) {
            mainState->MakeMine(this, GetPosition());
        }
    }
}

const Vector2 Ship::CalcNewBulletVelocity() const noexcept {
    return CalcBulletDirectionFromDifficulty() * _laserWeapon.GetSpeed();
}

const Vector2 Ship::CalcNewBulletPosition() const noexcept {
    return GetPosition() + GetForward() * GetCosmeticRadius();
}

const Vector2 Ship::CalcBulletDirectionFromDifficulty() const noexcept {
    const auto current_angle = GetForward().CalcHeadingDegrees();
    const auto angle_bias = []() {
        if(auto* game = GetGameAs<Game>(); game != nullptr) {
            switch(game->gameOptions.GetDifficulty()) {
            case Difficulty::Easy: return MathUtils::GetRandomNegOneToOne<float>() * 2.5f;
            case Difficulty::Normal: return MathUtils::GetRandomNegOneToOne<float>() * 5.0f;
            case Difficulty::Hard: return MathUtils::GetRandomNegOneToOne<float>() * 10.0f;
            default: return 0.0f;
            }
        }
        return 0.0f;
    }();
    const auto new_angle = current_angle + angle_bias;
    return Vector2::CreateFromPolarCoordinatesDegrees(1.0f, new_angle);
}

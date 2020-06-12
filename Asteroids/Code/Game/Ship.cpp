#include "Game/Ship.hpp"

#include "Engine/Core/Vertex3D.hpp"
#include "Engine/Core/Rgba.hpp"

#include "Engine/Math/Vector3.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"

#include "Game/GameCommon.hpp"

#include "Game/Asteroid.hpp"

#include <algorithm>

Ship::Ship() : Ship(Vector2::ZERO) {}

Ship::Ship(Vector2 position)
    : Entity()
{
    material = g_theRenderer->GetMaterial("ship");
    scoreValue = -100LL;
    SetPosition(position);
    SetCosmeticRadius(25.0f);
    SetPhysicalRadius(15.0f);
    _fireRate.SetFrequency(3u);
}

void Ship::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    Entity::Update(deltaSeconds);
    
    const auto uvs = AABB2::ZERO_TO_ONE;
    const auto tex = material->GetTexture(Material::TextureID::Diffuse);
    const auto frameWidth = static_cast<float>(tex->GetDimensions().x);
    const auto frameHeight = static_cast<float>(tex->GetDimensions().y);
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

void Ship::EndFrame() noexcept {
    if(_fireRate.CheckAndReset()) {
        _canFire = true;
    }
}

void Ship::OnDestroy() noexcept {
    Entity::OnDestroy();
    if(auto& ship = g_theGame->ship; ship && ship->IsDead()) {
        ship = nullptr;
    }
    g_theGame->MakeExplosion(GetPosition());
}

void Ship::OnFire() noexcept {
    if(_canFire) {
        _canFire = false;
        MakeBullet();
    }
}

void Ship::Thrust(float force) noexcept {
    AddForce(GetForward() * force);
}

void Ship::OnCollision(Entity* a, Entity* b) noexcept {
    const auto* asAsteroid = dynamic_cast<Asteroid*>(b);
    if(asAsteroid) {
        a->Kill();
    }
}

void Ship::OnCreate() noexcept {
    /* DO NOTHING */
}

void Ship::MakeBullet() const noexcept {
    g_theGame->MakeBullet(this, CalcNewBulletPosition(), CalcNewBulletVelocity());
}

const Vector2 Ship::CalcNewBulletVelocity() const noexcept {
    return GetForward() * (_minimumBulletSpeed + GetSpeed()) * _bulletSpeedFactor;
}

const Vector2 Ship::CalcNewBulletPosition() const noexcept {
    return GetPosition() + GetForward() * GetCosmeticRadius();
}


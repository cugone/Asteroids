#include "Game/Ship.hpp"

#include "Engine/Core/Vertex3D.hpp"
#include "Engine/Core/Rgba.hpp"

#include "Engine/Math/Vector3.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"

#include "Game/Asteroid.hpp"

#include <algorithm>

Ship::Ship() : Ship(Vector2::ZERO) {}

Ship::Ship(Vector2 position)
    : Entity()
{
    _thrust = std::move(std::make_unique<ThrustComponent>(this));
    material = g_theRenderer->GetMaterial("ship");
    scoreValue = -100LL;
    SetPosition(position);
    SetOrientationDegrees(-90.0f);
    SetCosmeticRadius(25.0f);
    SetPhysicalRadius(15.0f);
    _fireRate.SetFrequency(3u);
}

void Ship::BeginFrame() noexcept {
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

    _thrust->Update(deltaSeconds);

}

void Ship::Render(Renderer& renderer) const noexcept {
    _thrust->Render(renderer);
    Entity::Render(renderer);
}

void Ship::EndFrame() noexcept {
    Entity::EndFrame();
    if(_fireRate.CheckAndReset()) {
        _canFire = true;
    }
    _thrust->EndFrame();
}

void Ship::OnDestroy() noexcept {
    Entity::OnDestroy();
    g_theGame->MakeExplosion(GetPosition());
}

void Ship::OnFire() noexcept {
    if(_canFire) {
        _canFire = false;
        MakeBullet();
    }
}

void Ship::Thrust(float force) noexcept {
    _thrust->SetThrust(force);
    AddForce(GetForward() * force);
}

void Ship::OnCollision(Entity* a, Entity* b) noexcept {
    const auto* asAsteroid = dynamic_cast<Asteroid*>(b);
    if(asAsteroid) {
        a->DecrementHealth();
        if(a->IsDead()) {
            Thrust(0.0f);
            g_theGame->DecrementLives();
        }
    }
}

void Ship::OnCreate() noexcept {
    /* DO NOTHING */
}

void Ship::MakeBullet() const noexcept {
    g_theGame->MakeBullet(this, CalcNewBulletPosition(), CalcNewBulletVelocity());
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

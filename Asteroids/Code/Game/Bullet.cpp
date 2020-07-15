#include "Game/Bullet.hpp"

#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"

#include "Game/Asteroid.hpp"

#include <algorithm>

Bullet::Bullet(const Entity* parent, Vector2 position, Vector2 velocity) noexcept
: Entity()
, _parent(parent)
{
    SetPosition(position);
    SetVelocity(velocity);
    SetCosmeticRadius(15.0f);
    SetPhysicalRadius(10.0f);
    SetOrientationDegrees(_parent->GetOrientationDegrees());
    ttl.SetSeconds(TimeUtils::FPSeconds{10.0f});
}

void Bullet::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    Entity::Update(deltaSeconds);
    if(ttl.CheckAndReset()) {
        Kill();
        return;
    }

    const auto pos = GetPosition();
    const auto uvs = AABB2::ZERO_TO_ONE;
    const auto mat = g_theRenderer->GetMaterial("bullet");
    const auto tex = mat->GetTexture(Material::TextureID::Diffuse);
    const auto frameWidth = static_cast<float>(tex->GetDimensions().x);
    const auto frameHeight = static_cast<float>(tex->GetDimensions().y);
    const auto half_extents = Vector2{frameWidth, frameHeight} *0.5f;
    {
        const auto S = Matrix4::CreateScaleMatrix(half_extents);
        const auto R = Matrix4::Create2DRotationDegreesMatrix(90.0f + GetOrientationDegrees());
        const auto T = Matrix4::CreateTranslationMatrix(GetPosition());
        transform = Matrix4::MakeSRT(S, R, T);
    }

    if(g_theGame->IsEntityInView(this)) {
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
        builder.End(mat);
    }
}

void Bullet::EndFrame() noexcept {
    if(const auto& found = std::find(std::begin(g_theGame->bullets), std::end(g_theGame->bullets), this);
      (found != std::end(g_theGame->bullets) &&
      (*found)->IsDead()))
    {
        *found = nullptr;
    }
    Entity::EndFrame();
}

void Bullet::OnDestroy() noexcept {
    Entity::OnDestroy();
}

void Bullet::OnFire() noexcept {
    /* DO NOTHING */
}

void Bullet::OnCollision(Entity* a, Entity* b) noexcept {
    const auto* asAsteroid = dynamic_cast<Asteroid*>(b);
    if(asAsteroid) {
        a->Kill();
    }
}

void Bullet::OnCreate() noexcept {
    g_theAudioSystem->Play("Data/Audio/Sound/Laser_Shoot.wav");
}


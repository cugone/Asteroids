#include "Game/Bullet.hpp"

#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"

#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"
#include "Game/Game.hpp"

#include <algorithm>

Bullet::Bullet(const Entity* parent, Vector2 position, Vector2 velocity) noexcept
: Entity()
, _parent(parent)
{
    faction = _parent->faction;
    SetPosition(position);
    SetVelocity(velocity);
    SetCosmeticRadius(15.0f);
    SetPhysicalRadius(10.0f);
    SetOrientationDegrees(velocity.CalcHeadingDegrees());
    ttl.SetSeconds(TimeUtils::FPSeconds{CalculateTtlFromDifficulty()});
}

float Bullet::CalculateTtlFromDifficulty() const noexcept {
    switch(g_theGame->gameOptions.difficulty) {
    case Difficulty::Easy: return 3.0f;
    case Difficulty::Normal: return 2.0f;
    case Difficulty::Hard: return 1.0f;
    default: return 0.0f;
    }

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
        const auto R = Matrix4::Create2DRotationDegreesMatrix(GetOrientationDegrees());
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
    builder.End(mat);

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

void Bullet::OnCollision(Entity* /*a*/, Entity* /*b*/) noexcept {
    /* DO NOTHING */
}

void Bullet::OnCreate() noexcept {
    g_theAudioSystem->Play(g_sound_shootpath);
}


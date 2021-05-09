#include "Game/Bullet.hpp"

#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"

#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"
#include "Game/Game.hpp"

#include <algorithm>

Bullet::Bullet(const Entity* parent, a2de::Vector2 position, a2de::Vector2 velocity) noexcept
: Entity()
, _parent(parent)
{
    faction = _parent->faction;
    SetPosition(position);
    SetVelocity(velocity);
    SetCosmeticRadius(15.0f);
    SetPhysicalRadius(10.0f);
    SetOrientationDegrees(velocity.CalcHeadingDegrees());
    ttl.SetSeconds(a2de::TimeUtils::FPSeconds{CalculateTtlFromDifficulty()});
}

float Bullet::CalculateTtlFromDifficulty() const noexcept {
    switch(g_theGame->gameOptions.difficulty) {
    case Difficulty::Easy: return 3.0f;
    case Difficulty::Normal: return 2.0f;
    case Difficulty::Hard: return 1.0f;
    default: return 0.0f;
    }

}

void Bullet::Update(a2de::TimeUtils::FPSeconds deltaSeconds) noexcept {
    Entity::Update(deltaSeconds);
    if(ttl.CheckAndReset()) {
        Kill();
        return;
    }

    const auto pos = GetPosition();
    const auto uvs = a2de::AABB2::ZERO_TO_ONE;
    const auto mat = g_theRenderer->GetMaterial("bullet");
    const auto tex = mat->GetTexture(a2de::Material::TextureID::Diffuse);
    const auto frameWidth = static_cast<float>(tex->GetDimensions().x);
    const auto frameHeight = static_cast<float>(tex->GetDimensions().y);
    const auto half_extents = a2de::Vector2{frameWidth, frameHeight} *0.5f;
    {
        const auto S = a2de::Matrix4::CreateScaleMatrix(half_extents);
        const auto R = a2de::Matrix4::Create2DRotationDegreesMatrix(90.0f + GetOrientationDegrees());
        const auto T = a2de::Matrix4::CreateTranslationMatrix(GetPosition());
        transform = a2de::Matrix4::MakeSRT(S, R, T);
    }

    auto& builder = mesh_builder;
    builder.Begin(a2de::PrimitiveType::Triangles);
    builder.SetColor(a2de::Rgba::White);

    builder.SetUV(a2de::Vector2{uvs.maxs.x, uvs.maxs.y});
    builder.AddVertex(a2de::Vector2{+0.5f, +0.5f});

    builder.SetUV(a2de::Vector2{uvs.mins.x, uvs.maxs.y});
    builder.AddVertex(a2de::Vector2{-0.5f, +0.5f});

    builder.SetUV(a2de::Vector2{uvs.mins.x, uvs.mins.y});
    builder.AddVertex(a2de::Vector2{-0.5f, -0.5f});

    builder.SetUV(a2de::Vector2{uvs.maxs.x, uvs.mins.y});
    builder.AddVertex(a2de::Vector2{+0.5f, -0.5f});

    builder.AddIndicies(a2de::Mesh::Builder::Primitive::Quad);
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


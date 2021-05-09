#include "Game/Explosion.hpp"

#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Math/Disc2.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"

#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"
#include "Game/Game.hpp"

#include <algorithm>

Explosion::Explosion(a2de::Vector2 position)
: Entity()
{
    a2de::AnimatedSpriteDesc desc{};
    desc.material = g_theRenderer->GetMaterial("explosion");
    desc.spriteSheet = g_theGame->explosion_sheet;
    desc.durationSeconds = a2de::TimeUtils::FPSeconds{0.50f};
    desc.playbackMode = a2de::AnimatedSprite::SpriteAnimMode::Play_To_End;
    desc.frameLength = 25;
    desc.startSpriteIndex = 0;
    material = desc.material;
    _sprite = g_theRenderer->CreateAnimatedSprite(desc);

    SetPosition(position);
    const auto half_frameWidth = static_cast<float>(_sprite->GetFrameDimensions().x) * 0.5f;
    const auto half_frameHeight = static_cast<float>(_sprite->GetFrameDimensions().y) * 0.5f;
    SetCosmeticRadius((std::max)(half_frameWidth, half_frameHeight));
    SetPhysicalRadius(GetCosmeticRadius() * 0.8f);

}

void Explosion::Update(a2de::TimeUtils::FPSeconds deltaSeconds) noexcept {
    Entity::Update(deltaSeconds);
    _sprite->Update(deltaSeconds);

    const auto uvs = _sprite->GetCurrentTexCoords();
    const auto frameWidth = static_cast<float>(_sprite->GetFrameDimensions().x);
    const auto frameHeight = static_cast<float>(_sprite->GetFrameDimensions().y);
    const auto half_extents = a2de::Vector2{frameWidth, frameHeight};
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
    builder.End(material);

}

void Explosion::EndFrame() noexcept {
    Entity::EndFrame();
    if(_sprite->IsFinished()) {
        Kill();
    }

    if(const auto& found = std::find(std::begin(g_theGame->explosions), std::end(g_theGame->explosions), this);
        (found != std::end(g_theGame->explosions) &&
    (*found)->IsDead()))
    {
        *found = nullptr;
    }
}

void Explosion::OnDestroy() noexcept {
    Entity::OnDestroy();
}

void Explosion::OnFire() noexcept {
    /* DO NOTHING */
}

void Explosion::OnCollision(Entity* /*a*/, Entity* /*b*/) noexcept {
    /* DO NOTHING */
}

void Explosion::OnCreate() noexcept {
    g_theAudioSystem->Play(g_sound_explosionpath);
}

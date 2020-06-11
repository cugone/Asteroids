#include "Game/Explosion.hpp"

#include "Engine/Math/Disc2.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"

#include "Game/GameCommon.hpp"

#include <algorithm>

Explosion::Explosion(Vector2 position)
: Entity()
{
    SetPosition(position);
    AnimatedSpriteDesc desc{};
    desc.material = g_theRenderer->GetMaterial("explosion");
    desc.spriteSheet = g_theGame->explosion_sheet;
    desc.durationSeconds = TimeUtils::FPSeconds{0.50f};
    desc.playbackMode = AnimatedSprite::SpriteAnimMode::Play_To_End;
    desc.frameLength = 25;
    desc.startSpriteIndex = 0;
    _sprite = g_theRenderer->CreateAnimatedSprite(desc);
}

void Explosion::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    Entity::Update(deltaSeconds);
    _sprite->Update(deltaSeconds);

    const auto uvs = _sprite->GetCurrentTexCoords();
    const auto mat = _sprite->GetMaterial();
    const auto frameWidth = static_cast<float>(_sprite->GetFrameDimensions().x);
    const auto frameHeight = static_cast<float>(_sprite->GetFrameDimensions().y);
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
    builder.End(mat);
}

void Explosion::EndFrame() noexcept {

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
    g_theAudioSystem->Play("Data/Audio/Explosion.wav");
}

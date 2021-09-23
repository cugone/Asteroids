#include "Game/Bullet.hpp"

#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Core/EngineCommon.hpp"

#include "Engine/Game/GameBase.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"

#include "Engine/Scene/Components.hpp"

#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"
#include "Game/Game.hpp"

#include <algorithm>

Bullet::Bullet(const GameEntity* parent, Vector2 position, Vector2 velocity) noexcept
: GameEntity()
, _parent(parent)
{
    AddComponent<TransformComponent>();

    faction = _parent->faction;
    SetPosition(position);
    SetVelocity(velocity);
    SetCosmeticRadius(15.0f);
    SetPhysicalRadius(10.0f);
    SetOrientationDegrees(velocity.CalcHeadingDegrees());
    ttl.SetSeconds(TimeUtils::FPSeconds{CalculateTtlFromDifficulty()});
}

float Bullet::CalculateTtlFromDifficulty() const noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        switch(game->gameOptions.GetDifficulty()) {
        case Difficulty::Easy: return 3.0f;
        case Difficulty::Normal: return 2.0f;
        case Difficulty::Hard: return 1.0f;
        default: return 0.0f;
        }
    }
    return 0.0f;
}

void Bullet::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    GameEntity::Update(deltaSeconds);
    if(ttl.CheckAndReset()) {
        Kill();
        return;
    }

    const auto pos = GetPosition();
    const auto uvs = AABB2::Zero_to_One;
    const auto mat = GetMaterial();
    const auto tex = mat->GetTexture(Material::TextureID::Diffuse);
    const auto frameWidth = static_cast<float>(tex->GetDimensions().x);
    const auto frameHeight = static_cast<float>(tex->GetDimensions().y);
    const auto half_extents = Vector2{frameWidth, frameHeight} *0.5f;
    {
        const auto S = Matrix4::CreateScaleMatrix(half_extents);
        const auto R = Matrix4::Create2DRotationDegreesMatrix(GetOrientationDegrees());
        const auto T = Matrix4::CreateTranslationMatrix(GetPosition());
        auto& transform = GetComponent<TransformComponent>();
        transform.Transform = Matrix4::MakeSRT(S, R, T);
    }

    auto& builder = m_mesh_builder;
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
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        if(const auto& found = std::find(std::begin(game->bullets), std::end(game->bullets), this);
            (found != std::end(game->bullets) &&
                (*found)->IsDead()))
        {
            *found = nullptr;
        }
    }
    GameEntity::EndFrame();
}

void Bullet::OnDestroy() noexcept {
    GameEntity::OnDestroy();
}

Material* Bullet::GetMaterial() const noexcept {
    return g_theRenderer->GetMaterial("bullet");
}

void Bullet::OnFire() noexcept {
    /* DO NOTHING */
}

void Bullet::OnCollision(GameEntity* /*a*/, GameEntity* /*b*/) noexcept {
    /* DO NOTHING */
}

void Bullet::OnCreate() noexcept {
    AudioSystem::SoundDesc desc{};
    desc.groupName = g_audiogroup_sound;
    g_theAudioSystem->Play(g_sound_shootpath, desc);
}


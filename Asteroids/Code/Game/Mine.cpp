#include "Game/Mine.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"
#include "Game/Bullet.hpp"

Mine::Mine(const Entity* parent, a2de::Vector2 position)
    : Entity()
{
    faction = parent->faction;
    SetPosition(position);
    SetCosmeticRadius(25.0f);
    SetPhysicalRadius(25.0f);

    a2de::AnimatedSpriteDesc desc{};
    desc.material = g_theRenderer->GetMaterial("mine");
    desc.spriteSheet = GetSpriteSheet();
    desc.durationSeconds = a2de::TimeUtils::FPSeconds{1.0f};
    desc.playbackMode = a2de::AnimatedSprite::SpriteAnimMode::Looping;
    desc.frameLength = 12;
    desc.startSpriteIndex = 0;

    material = desc.material;

    _sprite = g_theRenderer->CreateAnimatedSprite(desc);

}

void Mine::Update(a2de::TimeUtils::FPSeconds deltaSeconds) noexcept {
    Entity::Update(deltaSeconds);
    _sprite->Update(deltaSeconds);

    const auto uvs = _sprite->GetCurrentTexCoords();
    const auto frameWidth = static_cast<float>(_sprite->GetFrameDimensions().x);
    const auto frameHeight = static_cast<float>(_sprite->GetFrameDimensions().y);
    const auto half_extents = a2de::Vector2{frameWidth, frameHeight};
    {
        const auto S = a2de::Matrix4::CreateScaleMatrix(half_extents);
        const auto R = a2de::Matrix4::I;
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

void Mine::EndFrame() noexcept {
    Entity::EndFrame();
    if(const auto& found = std::find(std::begin(g_theGame->mines), std::end(g_theGame->mines), this);
        (found != std::end(g_theGame->mines) &&
            (*found)->IsDead()))
    {
        *found = nullptr;
    }
}

void Mine::OnCreate() noexcept {
    /* DO NOTHING */
}

void Mine::OnCollision(Entity* /*a*/, Entity* /*b*/) noexcept {
    /* DO NOTHING */
}

void Mine::OnFire() noexcept {
    /* DO NOTHING */
}

void Mine::OnDestroy() noexcept {
    Entity::OnDestroy();
    g_theGame->MakeExplosion(GetPosition());
}

std::weak_ptr<a2de::SpriteSheet> Mine::GetSpriteSheet() const noexcept {
    return g_theGame->mine_sheet;
}
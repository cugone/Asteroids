#include "Game/Mine.hpp"

#include "Engine/Core/EngineCommon.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"
#include "Game/Bullet.hpp"

Mine::Mine(const Entity* parent, Vector2 position)
    : Entity()
{
    faction = parent->faction;
    SetPosition(position);
    SetCosmeticRadius(25.0f);
    SetPhysicalRadius(25.0f);

    AnimatedSpriteDesc desc{};
    desc.material = g_theRenderer->GetMaterial("mine");
    desc.spriteSheet = GetSpriteSheet();
    desc.durationSeconds = TimeUtils::FPSeconds{1.0f};
    desc.playbackMode = AnimatedSprite::SpriteAnimMode::Looping;
    desc.frameLength = 12;
    desc.startSpriteIndex = 0;

    _sprite = g_theRenderer->CreateAnimatedSprite(desc);

}

void Mine::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    Entity::Update(deltaSeconds);
    _sprite->Update(deltaSeconds);

    const auto uvs = _sprite->GetCurrentTexCoords();
    const auto frameWidth = static_cast<float>(_sprite->GetFrameDimensions().x);
    const auto frameHeight = static_cast<float>(_sprite->GetFrameDimensions().y);
    const auto half_extents = Vector2{frameWidth, frameHeight};
    {
        const auto S = Matrix4::CreateScaleMatrix(half_extents);
        const auto R = Matrix4::I;
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
    builder.End(GetMaterial());

}

void Mine::EndFrame() noexcept {
    Entity::EndFrame();
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        if(const auto& found = std::find(std::begin(game->mines), std::end(game->mines), this);
            (found != std::end(game->mines) &&
                (*found)->IsDead()))
        {
            *found = nullptr;
        }
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
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        game->MakeExplosion(GetPosition());
    }
}

Material* Mine::GetMaterial() const noexcept {
    return g_theRenderer->GetMaterial("mine");
}

std::weak_ptr<SpriteSheet> Mine::GetSpriteSheet() const noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        return game->mine_sheet;
    }
    return {};
}

#include "Game/LaserChargeComponent.hpp"

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/LineSegment2.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"
#include "Game/Asteroid.hpp"


LaserChargeComponent::LaserChargeComponent(Entity* parent)
    : Entity()
    , m_parent{parent}
{
    m_positionOffset = m_parent->GetForward() * m_parent->GetCosmeticRadius();
    m_ray.SetOrientationDegrees(m_parent->GetOrientationDegrees());


    AnimatedSpriteDesc desc{};
    desc.material = g_theRenderer->GetMaterial("laserchargeup");
    desc.spriteSheet = g_theGame->asteroid_sheet;
    desc.durationSeconds = TimeUtils::FPSeconds{1.0f};
    desc.playbackMode = AnimatedSprite::SpriteAnimMode::Play_To_End;
    desc.frameLength = 16;
    desc.startSpriteIndex = 0;

    material = desc.material;

    _sprite = g_theRenderer->CreateAnimatedSprite(desc);

}

void LaserChargeComponent::BeginFrame() noexcept {
    m_ray.SetOrientationDegrees(m_parent->GetOrientationDegrees());
    if(m_ischarging) {
        m_ischarging = !m_chargeTime.CheckAndReset();
    }
}

void LaserChargeComponent::Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(m_ischarging) {
        _sprite->Update(deltaSeconds);
        const auto frameWidth = static_cast<float>(_sprite->GetFrameDimensions().x);
        const auto frameHeight = static_cast<float>(_sprite->GetFrameDimensions().y);
        const auto half_extents = Vector2{frameWidth, frameHeight};
        auto forward = m_parent->GetForward();
        m_positionOffset = forward * m_parent->GetCosmeticRadius();
        const auto S = Matrix4::I;
        const auto R = Matrix4::I;
        const auto T = Matrix4::CreateTranslationMatrix(m_positionOffset);
        transform = Matrix4::MakeRT(m_parent->GetTransform(), Matrix4::MakeSRT(S, R, T));

        const auto uvs = _sprite->GetCurrentTexCoords();
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
}

void LaserChargeComponent::EndFrame() noexcept {
    /* DO NOTHING */
}

void LaserChargeComponent::OnCreate() noexcept {
    /* DO NOTHING */
}

bool LaserChargeComponent::DoneFiring() noexcept {
    return !m_ischarging;
}

void LaserChargeComponent::OnCollision(Entity* /*a*/, Entity* /*b*/) noexcept {
    /* DO NOTHING */
}

void LaserChargeComponent::OnFire() noexcept {
    /* DO NOTHING */
}

void LaserChargeComponent::OnDestroy() noexcept {
    /* DO NOTHING */
}

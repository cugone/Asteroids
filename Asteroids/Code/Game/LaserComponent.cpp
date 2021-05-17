#include "Game/LaserComponent.hpp"

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/LineSegment2.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"
#include "Game/Asteroid.hpp"


LaserComponent::LaserComponent(Entity* parent)
    : Entity()
    , m_parent{parent}
    , m_ray{}
{
    m_ray.position = m_parent->GetPosition() + (m_parent->GetForward() * m_parent->GetCosmeticRadius());
    m_ray.SetOrientationDegrees(m_parent->GetOrientationDegrees());
    material = g_theRenderer->GetMaterial("laser");
}

void LaserComponent::BeginFrame() noexcept {
    m_ray.SetOrientationDegrees(m_parent->GetOrientationDegrees());
    if(m_isfiring) {
        m_isfiring = !m_fireTime.CheckAndReset();
    }
}

void LaserComponent::Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(m_isfiring) {
        const auto tex = material->GetTexture(Material::TextureID::Diffuse);
        const auto frameWidth = static_cast<float>(tex->GetDimensions().x);
        const auto frameHeight = static_cast<float>(tex->GetDimensions().y);
        const auto half_extents = Vector2{frameWidth, frameHeight} * 0.5f;
        auto forward = m_parent->GetForward();
        m_positionOffset = forward * m_parent->GetCosmeticRadius();
        const auto S = Matrix4::CreateScaleMatrix(Vector2{m_parent->GetCosmeticRadius() * 0.2f, frameHeight});
        const auto R = Matrix4::Create2DRotationDegreesMatrix(90.f + m_ray.direction.CalcHeadingDegrees());
        const auto T = Matrix4::CreateTranslationMatrix(m_positionOffset + Vector2{half_extents.x, half_extents.y});
        transform = Matrix4::MakeRT(m_parent->GetTransform(), Matrix4::MakeSRT(S, R, T));

        const auto uvs = AABB2::ZERO_TO_ONE;
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

        for(auto& asteroid : g_theGame->asteroids) {
            if(MathUtils::DoLineSegmentOverlap(Disc2{asteroid->GetPosition(), asteroid->GetPhysicalRadius()}, LineSegment2{m_ray.position, m_ray.direction, 1000.0f})) {
                asteroid->DecrementHealth();
            }
        }
    }
}

void LaserComponent::EndFrame() noexcept {
    /* DO NOTHING */
}

void LaserComponent::OnCreate() noexcept {
    /* DO NOTHING */
}

bool LaserComponent::DoneFiring() noexcept {
    return !m_isfiring;
}

void LaserComponent::OnCollision(Entity* /*a*/, Entity* /*b*/) noexcept {
    /* DO NOTHING */
}

void LaserComponent::OnFire() noexcept {
    /* DO NOTHING */
}

void LaserComponent::OnDestroy() noexcept {
    /* DO NOTHING */
}
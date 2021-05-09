#include "Game/ThrustComponent.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"

#include "Game/GameCommon.hpp"

ThrustComponent::ThrustComponent(Entity* parent, float maxThrust /*= 100.0f*/)
    : Entity()
    , m_parent(parent)
    , m_maxThrust(maxThrust)
{
    material = g_theRenderer->GetMaterial("thrust");
    SetCosmeticRadius(7.0f);
}

void ThrustComponent::Update([[maybe_unused]] a2de::TimeUtils::FPSeconds deltaSeconds) noexcept {
    const auto tex = material->GetTexture(a2de::Material::TextureID::Diffuse);
    const auto frameWidth = static_cast<float>(tex->GetDimensions().x);
    const auto frameHeight = static_cast<float>(tex->GetDimensions().y);
    const auto half_extents = a2de::Vector2{frameWidth, frameHeight};
    auto backward = m_parent->GetBackward();
    m_positionOffset = backward * (m_parent->GetCosmeticRadius() + GetCosmeticRadius());
    const auto S = a2de::Matrix4::I;
    const auto R = a2de::Matrix4::Create2DRotationDegreesMatrix(m_thrustDirectionAngleOffset);
    const auto T = a2de::Matrix4::CreateTranslationMatrix(m_positionOffset);
    //const auto R = Matrix4::Create2DRotationDegreesMatrix(m_parent->GetOrientationDegrees());
    //const auto T = Matrix4::CreateTranslationMatrix(m_parent->GetPosition());
    transform = a2de::Matrix4::MakeRT(m_parent->GetTransform(), a2de::Matrix4::MakeSRT(S, R, T));
    //transform = Matrix4::MakeSRT(S, R, T);

    if(m_thrust) {
        const auto uvs = a2de::AABB2::ZERO_TO_ONE;
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

}

void ThrustComponent::OnCreate() noexcept {
    /* DO NOTHING */
}

void ThrustComponent::OnCollision(Entity* /*a*/, Entity* /*b*/) noexcept {
    /* DO NOTHING */
}

void ThrustComponent::OnFire() noexcept {
    /* DO NOTHING */
}

void ThrustComponent::OnDestroy() noexcept {
    /* DO NOTHING */
}

void ThrustComponent::SetThrust(float thrust) noexcept {
    m_thrust = thrust;
}

float ThrustComponent::GetThrust(float thrust) const noexcept {
    return thrust;
}

float ThrustComponent::GetMaxThrust() const noexcept {
    return m_maxThrust;
}

void ThrustComponent::SetMaxThrust(float newMaxThrust) noexcept {
    m_maxThrust = newMaxThrust;
}

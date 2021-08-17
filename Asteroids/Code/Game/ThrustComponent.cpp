#include "Game/ThrustComponent.hpp"

#include "Engine/Core/EngineCommon.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include "Game/GameCommon.hpp"

ThrustComponent::ThrustComponent(Entity* parent, float maxThrust /*= 100.0f*/)
    : Entity()
    , m_parent(parent)
    , m_maxThrust(maxThrust)
{
    material = g_theRenderer->GetMaterial("thrust");
    SetCosmeticRadius(7.0f);
}

void ThrustComponent::BeginFrame() noexcept {
    m_thrustPS.BeginFrame();
    mesh_builder.Clear();
}

void ThrustComponent::Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    auto& rs = ServiceLocator::get<IRendererService>();
    m_thrustPS.Update(rs.GetGameTime().count(), deltaSeconds.count());

    const auto tex = material->GetTexture(Material::TextureID::Diffuse);
    const auto frameWidth = static_cast<float>(tex->GetDimensions().x);
    const auto frameHeight = static_cast<float>(tex->GetDimensions().y);
    const auto half_extents = Vector2{frameWidth, frameHeight};
    auto backward = m_parent->GetBackward();
    m_positionOffset = backward * (m_parent->GetCosmeticRadius() + GetCosmeticRadius());
    const auto S = Matrix4::I;
    const auto R = Matrix4::Create2DRotationDegreesMatrix(m_thrustDirectionAngleOffset);
    const auto T = Matrix4::CreateTranslationMatrix(m_positionOffset);
    //const auto R = Matrix4::Create2DRotationDegreesMatrix(m_parent->GetOrientationDegrees());
    //const auto T = Matrix4::CreateTranslationMatrix(m_parent->GetPosition());
    transform = Matrix4::MakeRT(m_parent->GetTransform(), Matrix4::MakeSRT(S, R, T));
    //transform = Matrix4::MakeSRT(S, R, T);

    if(m_thrust) {
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
    }

}

void ThrustComponent::Render() const noexcept {
    m_thrustPS.Render();
    auto& rs = ServiceLocator::get<IRendererService>();
    rs.SetModelMatrix(transform);
    Mesh::Render(mesh_builder);

}

void ThrustComponent::EndFrame() noexcept {
    m_thrustPS.EndFrame();
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
    if(m_thrust) {
        m_thrustPS.SetPlay(true);
    } else {
        m_thrustPS.Stop();
    }
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

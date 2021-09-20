#include "Game/Entity.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/MathUtils.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"

void Entity::BeginFrame() noexcept {
    m_mesh_builder.Clear();
}

void Entity::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    auto accel = CalcAcceleration();
    auto vel = GetVelocity();
    auto pos = GetPosition();
    const auto r = GetCosmeticRadius();
    Vector2 new_accel = accel;
    Vector2 new_vel = vel + new_accel * deltaSeconds.count();
    Vector2 new_pos = pos + new_vel * deltaSeconds.count();
    m_position_orientation_speed.x = new_pos.x;
    m_position_orientation_speed.y = new_pos.y;
    m_position_orientation_speed.w = new_vel.CalcLength();
    auto new_direction = Vector2::X_Axis;
    new_direction.SetUnitLengthAndHeadingDegrees(new_vel.CalcHeadingDegrees());
    m_cosmeticphysicalradius_velocitydirection.z = new_direction.x;
    m_cosmeticphysicalradius_velocitydirection.w = new_direction.y;
    m_acceleration_force.x = new_accel.x;
    m_acceleration_force.y = new_accel.y;
}

void Entity::Render() const noexcept {
    ServiceLocator::get<IRendererService>().SetModelMatrix(m_transform);
    Mesh::Render(m_mesh_builder);
}

void Entity::EndFrame() noexcept {
    ClearForce();
}

Vector2 Entity::GetForward() const noexcept {
    auto front = Vector2::X_Axis;
    front.SetHeadingDegrees(GetOrientationDegrees());
    return front;
}

Vector2 Entity::GetBackward() const noexcept {
    return -GetForward();
}

Vector2 Entity::GetRight() const noexcept {
    return GetForward().GetRightHandNormal();
}

Vector2 Entity::GetLeft() const noexcept {
    return GetForward().GetLeftHandNormal();
}

float Entity::GetCosmeticRadius() const noexcept {
    return m_cosmeticphysicalradius_velocitydirection.x;
}

float Entity::GetPhysicalRadius() const noexcept {
    return m_cosmeticphysicalradius_velocitydirection.y;
}

float Entity::GetSpeed() const noexcept {
    return m_position_orientation_speed.w;
}

void Entity::Kill() noexcept {
    m_invmass_rotationspeed_health_padding.z = 0;
}

bool Entity::IsDead() const noexcept {
    return m_invmass_rotationspeed_health_padding.z <= 0;
}

const Matrix4& Entity::GetTransform() const noexcept {
    return m_transform;
}

void Entity::DecrementHealth() noexcept {
    if(!IsDead()) {
        --m_invmass_rotationspeed_health_padding.z;
    } else {
        Kill();
    }
}

void Entity::SetHealth(int newHealth) noexcept {
    m_invmass_rotationspeed_health_padding.z = static_cast<float>(newHealth);
}

void Entity::SetPosition(Vector2 newPosition) noexcept {
    m_position_orientation_speed.x = newPosition.x;
    m_position_orientation_speed.y = newPosition.y;
}

Vector2 Entity::GetPosition() const noexcept {
    return m_position_orientation_speed.GetXY();
}

void Entity::SetVelocity(Vector2 newVelocity) noexcept {
    m_position_orientation_speed.w = newVelocity.Normalize();
    m_cosmeticphysicalradius_velocitydirection.z = newVelocity.x;
    m_cosmeticphysicalradius_velocitydirection.w = newVelocity.y;
}

Vector2 Entity::GetVelocity() const noexcept {
    return m_cosmeticphysicalradius_velocitydirection.GetZW() * m_position_orientation_speed.w;
}

Vector2 Entity::GetAcceleration() const noexcept {
    return m_acceleration_force.GetXY();
}

Vector2 Entity::CalcAcceleration() noexcept {
    return m_acceleration_force.GetZW() * GetInvMass();
}

Vector2 Entity::GetForce() const noexcept {
    return Vector2{m_acceleration_force.z, m_acceleration_force.w};
}

void Entity::AddForce(const Vector2& force) noexcept {
    m_acceleration_force.z += force.x;
    m_acceleration_force.w += force.y;
}

void Entity::ClearForce() noexcept {
    m_acceleration_force.z = 0.0f;
    m_acceleration_force.w = 0.0f;
}

float Entity::GetMass() const noexcept {
    return 1.0f / GetInvMass();
}

float Entity::GetInvMass() const noexcept {
    return m_invmass_rotationspeed_health_padding.x;
}

void Entity::SetOrientationDegrees(float newDegrees) noexcept {
    m_position_orientation_speed.z = newDegrees;
}

void Entity::SetOrientationRadians(float newRadians) noexcept {
    SetOrientationDegrees(MathUtils::ConvertRadiansToDegrees(newRadians));
}

float Entity::GetOrientationDegrees() const noexcept {
    return m_position_orientation_speed.z;
}

float Entity::GetOrientationRadians() const noexcept {
    return MathUtils::ConvertDegreesToRadians(GetOrientationDegrees());
}

void Entity::OnDestroy() noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        game->player.AdjustScore(scoreValue);
    }
}

void Entity::RotateCounterClockwise(float speed) noexcept {
    AdjustOrientation(speed);
}

void Entity::RotateClockwise(float speed) noexcept {
    AdjustOrientation(-speed);
}

void Entity::AdjustOrientation(float value) noexcept {
    m_position_orientation_speed.z += value;
    m_position_orientation_speed.z = MathUtils::Wrap(m_position_orientation_speed.z, 0.0f, 360.0f);
}

float Entity::GetRotationSpeed() const noexcept {
    return m_invmass_rotationspeed_health_padding.y;
}

void Entity::SetRotationSpeed(float speed) noexcept {
    m_invmass_rotationspeed_health_padding.y = speed;
}

IWeapon* Entity::GetWeapon() const noexcept {
    return m_weapon;
}

void Entity::SetCosmeticRadius(float value) noexcept {
    m_cosmeticphysicalradius_velocitydirection.x = value;
}

void Entity::SetPhysicalRadius(float value) noexcept {
    m_cosmeticphysicalradius_velocitydirection.y = value;
}

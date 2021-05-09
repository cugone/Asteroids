#include "Game/Entity.hpp"

#include "Engine/Math/MathUtils.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"

void Entity::BeginFrame() noexcept {
    mesh_builder.Clear();
}

void Entity::Update(a2de::TimeUtils::FPSeconds deltaSeconds) noexcept {
    auto accel = CalcAcceleration();
    auto vel = GetVelocity();
    auto pos = GetPosition();
    const auto r = GetCosmeticRadius();
    a2de::Vector2 new_accel = accel;
    a2de::Vector2 new_vel = vel + new_accel * deltaSeconds.count();
    a2de::Vector2 new_pos = pos + new_vel * deltaSeconds.count();
    position_orientation_speed.x = new_pos.x;
    position_orientation_speed.y = new_pos.y;
    position_orientation_speed.w = new_vel.CalcLength();
    auto new_direction = a2de::Vector2::X_AXIS;
    new_direction.SetUnitLengthAndHeadingDegrees(new_vel.CalcHeadingDegrees());
    cosmeticphysicalradius_velocitydirection.z = new_direction.x;
    cosmeticphysicalradius_velocitydirection.w = new_direction.y;
    acceleration_force.x = new_accel.x;
    acceleration_force.y = new_accel.y;
}

void Entity::Render(a2de::Renderer& renderer) const noexcept {
    renderer.SetModelMatrix(transform);
    a2de::Mesh::Render(renderer, mesh_builder);
}

void Entity::EndFrame() noexcept {
    ClearForce();
}

a2de::Vector2 Entity::GetForward() const noexcept {
    auto front = a2de::Vector2::X_AXIS;
    front.SetHeadingDegrees(GetOrientationDegrees());
    return front;
}

a2de::Vector2 Entity::GetBackward() const noexcept {
    return -GetForward();
}

a2de::Vector2 Entity::GetRight() const noexcept {
    return GetForward().GetRightHandNormal();
}

a2de::Vector2 Entity::GetLeft() const noexcept {
    return GetForward().GetLeftHandNormal();
}

float Entity::GetCosmeticRadius() const noexcept {
    return cosmeticphysicalradius_velocitydirection.x;
}

float Entity::GetPhysicalRadius() const noexcept {
    return cosmeticphysicalradius_velocitydirection.y;
}

float Entity::GetSpeed() const noexcept {
    return position_orientation_speed.w;
}

void Entity::Kill() noexcept {
    invmass_rotationspeed_health_padding.z = 0;
}

bool Entity::IsDead() const noexcept {
    return invmass_rotationspeed_health_padding.z <= 0;
}

const a2de::Matrix4& Entity::GetTransform() const noexcept {
    return transform;
}

a2de::Material* Entity::GetMaterial() const noexcept {
    return material;
}

void Entity::DecrementHealth() noexcept {
    if(!IsDead()) {
        --invmass_rotationspeed_health_padding.z;
    } else {
        Kill();
    }
}

void Entity::SetHealth(int newHealth) noexcept {
    invmass_rotationspeed_health_padding.z = static_cast<float>(newHealth);
}

void Entity::SetPosition(a2de::Vector2 newPosition) noexcept {
    position_orientation_speed.x = newPosition.x;
    position_orientation_speed.y = newPosition.y;
}

a2de::Vector2 Entity::GetPosition() const noexcept {
    return position_orientation_speed.GetXY();
}

void Entity::SetVelocity(a2de::Vector2 newVelocity) noexcept {
    position_orientation_speed.w = newVelocity.Normalize();
    cosmeticphysicalradius_velocitydirection.z = newVelocity.x;
    cosmeticphysicalradius_velocitydirection.w = newVelocity.y;
}

a2de::Vector2 Entity::GetVelocity() const noexcept {
    return cosmeticphysicalradius_velocitydirection.GetZW() * position_orientation_speed.w;
}

a2de::Vector2 Entity::GetAcceleration() const noexcept {
    return acceleration_force.GetXY();
}

a2de::Vector2 Entity::CalcAcceleration() noexcept {
    return acceleration_force.GetZW() * GetInvMass();
}

a2de::Vector2 Entity::GetForce() const noexcept {
    return a2de::Vector2{acceleration_force.z, acceleration_force.w};
}

void Entity::AddForce(const a2de::Vector2& force) noexcept {
    acceleration_force.z += force.x;
    acceleration_force.w += force.y;
}

void Entity::ClearForce() noexcept {
    acceleration_force.z = 0.0f;
    acceleration_force.w = 0.0f;
}

float Entity::GetMass() const noexcept {
    return 1.0f / GetInvMass();
}

float Entity::GetInvMass() const noexcept {
    return invmass_rotationspeed_health_padding.x;
}

void Entity::SetOrientationDegrees(float newDegrees) noexcept {
    position_orientation_speed.z = newDegrees;
}

void Entity::SetOrientationRadians(float newRadians) noexcept {
    SetOrientationDegrees(a2de::MathUtils::ConvertRadiansToDegrees(newRadians));
}

float Entity::GetOrientationDegrees() const noexcept {
    return position_orientation_speed.z;
}

float Entity::GetOrientationRadians() const noexcept {
    return a2de::MathUtils::ConvertDegreesToRadians(GetOrientationDegrees());
}

void Entity::OnDestroy() noexcept {
    g_theGame->player.AdjustScore(scoreValue);
}

void Entity::RotateCounterClockwise(float speed) noexcept {
    AdjustOrientation(speed);
}

void Entity::RotateClockwise(float speed) noexcept {
    AdjustOrientation(-speed);
}

void Entity::AdjustOrientation(float value) noexcept {
    position_orientation_speed.z += value;
    position_orientation_speed.z = a2de::MathUtils::Wrap(position_orientation_speed.z, 0.0f, 360.0f);
}

float Entity::GetRotationSpeed() const noexcept {
    return invmass_rotationspeed_health_padding.y;
}

void Entity::SetRotationSpeed(float speed) noexcept {
    invmass_rotationspeed_health_padding.y = speed;
}

void Entity::SetCosmeticRadius(float value) noexcept {
    cosmeticphysicalradius_velocitydirection.x = value;
}

void Entity::SetPhysicalRadius(float value) noexcept {
    cosmeticphysicalradius_velocitydirection.y = value;
}

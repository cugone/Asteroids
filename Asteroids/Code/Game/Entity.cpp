#include "Game/Entity.hpp"

#include "Engine/Math/MathUtils.hpp"

#include "Game/GameCommon.hpp"


void Entity::BeginFrame() noexcept {
    mesh_builder.Clear();
}

void Entity::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    WrapAroundWorld();
    auto accel = CalcAcceleration();
    auto vel = GetVelocity();
    auto pos = GetPosition();
    const auto r = GetCosmeticRadius();
    Vector2 new_accel = GetAcceleration();
    Vector2 new_vel = vel + accel * deltaSeconds.count();
    Vector2 new_pos = pos + new_vel * deltaSeconds.count();
    position_orientation_speed.x = new_pos.x;
    position_orientation_speed.y = new_pos.y;
    position_orientation_speed.w = new_vel.CalcLength();
    auto new_direction = Vector2::X_AXIS;
    new_direction.SetUnitLengthAndHeadingDegrees(new_vel.CalcHeadingDegrees());
    cosmeticphysicalradius_velocitydirection.z = new_direction.x;
    cosmeticphysicalradius_velocitydirection.w = new_direction.y;
    acceleration_force.x = new_accel.x;
    acceleration_force.y = new_accel.y;
}

void Entity::Render(Renderer& renderer) const noexcept {
    renderer.SetModelMatrix(transform);
    Mesh::Render(renderer, mesh_builder);
}

void Entity::EndFrame() noexcept {
    ClearForce();
}

void Entity::WrapAroundWorld() noexcept {
    const auto world_left = g_theGame->world_bounds.mins.x;
    const auto world_right = g_theGame->world_bounds.maxs.x;
    const auto world_top = g_theGame->world_bounds.mins.y;
    const auto world_bottom = g_theGame->world_bounds.maxs.y;
    const auto r = GetCosmeticRadius();
    auto pos = GetPosition();
    const auto ship_right = pos.x + r;
    const auto ship_left = pos.x - r;
    const auto ship_top = pos.y - r;
    const auto ship_bottom = pos.y + r;
    const auto d = 2.0f * r;
    const auto world_width = g_theGame->world_bounds.CalcDimensions().x;
    const auto world_height = g_theGame->world_bounds.CalcDimensions().y;
    if(ship_right < world_left) {
        pos.x += d + world_width;
    }
    if(ship_left > world_right) {
        pos.x -= d + world_width;
    }
    if(ship_bottom < world_top) {
        pos.y += d + world_height;
    }
    if(ship_top > world_bottom) {
        pos.y -= d + world_height;
    }
    position_orientation_speed.x = pos.x;
    position_orientation_speed.y = pos.y;
}

Vector2 Entity::GetForward() const noexcept {
    Vector2 front = Vector2::X_AXIS;
    front.SetHeadingDegrees(GetOrientationDegrees());
    return front;
}

Vector2 Entity::GetRight() const noexcept {
    return GetForward().GetRightHandNormal();
}

Vector2 Entity::GetLeft() const noexcept {
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
    health = 0;
}

bool Entity::IsDead() const noexcept {
    return health <= 0;
}

const Matrix4& Entity::GetTransform() const noexcept {
    return transform;
}

Material* Entity::GetMaterial() const noexcept {
    return material;
}

void Entity::SetPosition(Vector2 newPosition) noexcept {
    position_orientation_speed.x = newPosition.x;
    position_orientation_speed.y = newPosition.y;
}

Vector2 Entity::GetPosition() const noexcept {
    return position_orientation_speed.GetXY();
}

void Entity::SetVelocity(Vector2 newVelocity) noexcept {
    position_orientation_speed.w = newVelocity.Normalize();
    cosmeticphysicalradius_velocitydirection.z = newVelocity.x;
    cosmeticphysicalradius_velocitydirection.w = newVelocity.y;
}

Vector2 Entity::GetVelocity() const noexcept {
    return cosmeticphysicalradius_velocitydirection.GetZW() * position_orientation_speed.w;
}

Vector2 Entity::GetAcceleration() const noexcept {
    return acceleration_force.GetXY();
}

Vector2 Entity::CalcAcceleration() noexcept {
    return acceleration_force.GetZW() * inv_mass;
}

void Entity::AddForce(const Vector2& force) noexcept {
    acceleration_force.z += force.x;
    acceleration_force.w += force.y;
}

void Entity::ClearForce() noexcept {
    acceleration_force.z = 0.0f;
    acceleration_force.w = 0.0f;
}

float Entity::GetMass() const noexcept {
    return 1.0f / inv_mass;
}

float Entity::GetInvMass() const noexcept {
    return inv_mass;
}

void Entity::SetOrientationDegrees(float newDegrees) noexcept {
    position_orientation_speed.z = newDegrees;
}

void Entity::SetOrientationRadians(float newRadians) noexcept {
    SetOrientationDegrees(MathUtils::ConvertRadiansToDegrees(newRadians));
}

float Entity::GetOrientationDegrees() const noexcept {
    return position_orientation_speed.z;
}

float Entity::GetOrientationRadians() const noexcept {
    return MathUtils::ConvertDegreesToRadians(GetOrientationDegrees());
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
    position_orientation_speed.z = MathUtils::Wrap(position_orientation_speed.z, 0.0f, 360.0f);
}

float Entity::GetRotationSpeed() const noexcept {
    return rotation_speed;
}

void Entity::SetRotationSpeed(float speed) noexcept {
    rotation_speed = speed;
}

void Entity::SetCosmeticRadius(float value) noexcept {
    cosmeticphysicalradius_velocitydirection.x = value;
}

void Entity::SetPhysicalRadius(float value) noexcept {
    cosmeticphysicalradius_velocitydirection.y = value;
}

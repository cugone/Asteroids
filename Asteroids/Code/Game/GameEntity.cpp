#include "Game/GameEntity.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/MathUtils.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"

#include "Engine/Scene/Components.hpp"

GameEntity::GameEntity(uint32_t handle, std::weak_ptr<Scene> scene) noexcept
: Entity(handle, scene)
{
    AddComponent<TransformComponent>(Matrix4::I);
    AddComponent<MeshComponent>(Mesh{});
}

void GameEntity::BeginFrame() noexcept {
    m_mesh_builder.Clear();
}

void GameEntity::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    auto accel = CalcAcceleration();
    auto vel = GetVelocity();
    auto pos = GetPosition();
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

void GameEntity::Render() const noexcept {
    ServiceLocator::get<IRendererService>()->SetModelMatrix(GetTransform());
    Mesh::Render(m_mesh_builder);
}

void GameEntity::EndFrame() noexcept {
    ClearForce();
}

Vector2 GameEntity::GetForward() const noexcept {
    auto front = Vector2::X_Axis;
    front.SetHeadingDegrees(GetOrientationDegrees());
    return front;
}

Vector2 GameEntity::GetBackward() const noexcept {
    return -GetForward();
}

Vector2 GameEntity::GetRight() const noexcept {
    return GetForward().GetRightHandNormal();
}

Vector2 GameEntity::GetLeft() const noexcept {
    return GetForward().GetLeftHandNormal();
}

float GameEntity::GetCosmeticRadius() const noexcept {
    return m_cosmeticphysicalradius_velocitydirection.x;
}

float GameEntity::GetPhysicalRadius() const noexcept {
    return m_cosmeticphysicalradius_velocitydirection.y;
}

float GameEntity::GetSpeed() const noexcept {
    return m_position_orientation_speed.w;
}

void GameEntity::Kill() noexcept {
    m_invmass_rotationspeed_health_padding.z = 0;
}

bool GameEntity::IsDead() const noexcept {
    return m_invmass_rotationspeed_health_padding.z <= 0;
}

const Matrix4& GameEntity::GetTransform() const noexcept {
    return GetComponent<TransformComponent>();
}

Matrix4& GameEntity::GetTransform() noexcept {
    return GetComponent<TransformComponent>();
}

void GameEntity::DecrementHealth() noexcept {
    if(!IsDead()) {
        --m_invmass_rotationspeed_health_padding.z;
    } else {
        Kill();
    }
}

void GameEntity::SetHealth(int newHealth) noexcept {
    m_invmass_rotationspeed_health_padding.z = static_cast<float>(newHealth);
}

void GameEntity::SetPosition(Vector2 newPosition) noexcept {
    m_position_orientation_speed.x = newPosition.x;
    m_position_orientation_speed.y = newPosition.y;
}

Vector2 GameEntity::GetPosition() const noexcept {
    return m_position_orientation_speed.GetXY();
}

void GameEntity::SetVelocity(Vector2 newVelocity) noexcept {
    m_position_orientation_speed.w = newVelocity.Normalize();
    m_cosmeticphysicalradius_velocitydirection.z = newVelocity.x;
    m_cosmeticphysicalradius_velocitydirection.w = newVelocity.y;
}

Vector2 GameEntity::GetVelocity() const noexcept {
    return m_cosmeticphysicalradius_velocitydirection.GetZW() * m_position_orientation_speed.w;
}

Vector2 GameEntity::GetAcceleration() const noexcept {
    return m_acceleration_force.GetXY();
}

Vector2 GameEntity::CalcAcceleration() noexcept {
    return m_acceleration_force.GetZW() * GetInvMass();
}

Vector2 GameEntity::GetForce() const noexcept {
    return Vector2{m_acceleration_force.z, m_acceleration_force.w};
}

void GameEntity::AddForce(const Vector2& force) noexcept {
    m_acceleration_force.z += force.x;
    m_acceleration_force.w += force.y;
}

void GameEntity::ClearForce() noexcept {
    m_acceleration_force.z = 0.0f;
    m_acceleration_force.w = 0.0f;
}

float GameEntity::GetMass() const noexcept {
    return 1.0f / GetInvMass();
}

float GameEntity::GetInvMass() const noexcept {
    return m_invmass_rotationspeed_health_padding.x;
}

void GameEntity::SetOrientationDegrees(float newDegrees) noexcept {
    m_position_orientation_speed.z = newDegrees;
}

void GameEntity::SetOrientationRadians(float newRadians) noexcept {
    SetOrientationDegrees(MathUtils::ConvertRadiansToDegrees(newRadians));
}

float GameEntity::GetOrientationDegrees() const noexcept {
    return m_position_orientation_speed.z;
}

float GameEntity::GetOrientationRadians() const noexcept {
    return MathUtils::ConvertDegreesToRadians(GetOrientationDegrees());
}

void GameEntity::OnDestroy() noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        game->player.AdjustScore(scoreValue);
    }
}

void GameEntity::RotateCounterClockwise(float speed) noexcept {
    AdjustOrientation(speed);
}

void GameEntity::RotateClockwise(float speed) noexcept {
    AdjustOrientation(-speed);
}

void GameEntity::AdjustOrientation(float value) noexcept {
    m_position_orientation_speed.z += value;
    m_position_orientation_speed.z = MathUtils::Wrap(m_position_orientation_speed.z, 0.0f, 360.0f);
}

float GameEntity::GetRotationSpeed() const noexcept {
    return m_invmass_rotationspeed_health_padding.y;
}

void GameEntity::SetRotationSpeed(float speed) noexcept {
    m_invmass_rotationspeed_health_padding.y = speed;
}

IWeapon* GameEntity::GetWeapon() const noexcept {
    return m_weapon;
}

void GameEntity::SetCosmeticRadius(float value) noexcept {
    m_cosmeticphysicalradius_velocitydirection.x = value;
}

void GameEntity::SetPhysicalRadius(float value) noexcept {
    m_cosmeticphysicalradius_velocitydirection.y = value;
}

bool GameEntity::HasGameParent() const noexcept {
    return m_gameParent != nullptr;
}

const GameEntity* GameEntity::GetGameParent() const noexcept {
    return m_gameParent;
}

GameEntity* GameEntity::GetGameParent() noexcept {
    return m_gameParent;
}

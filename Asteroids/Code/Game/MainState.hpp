#pragma once

#include "Engine/Core/TypeUtils.hpp"
#include "Engine/Core/OrthographicCameraController.hpp"

#include "Engine/Math/AABB2.hpp"

#include "Engine/Renderer/Camera2D.hpp"

#include "Game/GameState.hpp"
#include "Game/Player.hpp"

#include <memory>
#include <vector>

class Asteroid;
class Bullet;
class Explosion;
class Ship;
class Entity;

class MainState : public GameState {
public:
    virtual ~MainState() = default;

    void OnEnter() noexcept override;
    void OnExit() noexcept override;
    void BeginFrame() noexcept override;
    void Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) override;
    void Render() const noexcept override;
    void EndFrame() noexcept override;

protected:
private:
    std::unique_ptr<GameState> HandleInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept override;
    std::unique_ptr<GameState> HandleKeyboardInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    std::unique_ptr<GameState> HandleControllerInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    std::unique_ptr<GameState> HandleMouseInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;

    void HandleDebugInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds);
    void HandleDebugKeyboardInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds);
    void HandlePlayerInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds);
    void ClampCameraToWorld() noexcept;

    void WrapAroundWorld(Entity* e) noexcept;
    void UpdateEntities(TimeUtils::FPSeconds deltaSeconds) noexcept;
    void StartNewWave(unsigned int wave_number) noexcept;

    void MakeShip() noexcept;
    void MakeLargeAsteroidAtMouse() noexcept;

    void Respawn() noexcept;

    void HandleBulletAsteroidCollision() const noexcept;
    void HandleShipAsteroidCollision() noexcept;

    void KillAll() noexcept;

    unsigned int GetWaveMultiplierFromDifficulty() const noexcept;
    long long GetLivesFromDifficulty() const noexcept;

    void RenderBackground() const noexcept;
    void RenderEntities() const noexcept;
    void DebugRenderEntities() const noexcept;

    AABB2 CalculateCameraBounds() const noexcept;

    void RenderStatus() const noexcept;

    mutable Ship* ship{nullptr};
    AABB2 world_bounds = AABB2::ZERO_TO_ONE;

    mutable Camera2D m_ui_camera{};
    OrthographicCameraController m_cameraController{};
    float m_thrust_force{100.0f};
    bool m_debug_render{false};
};

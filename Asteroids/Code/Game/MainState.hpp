#pragma once

#include "Engine/Core/TypeUtils.hpp"
#include "Engine/Core/OrthographicCameraController.hpp"
#include "Engine/Core/Stopwatch.hpp"

#include "Engine/Math/AABB2.hpp"

#include "Engine/Renderer/Camera2D.hpp"

#include "Game/GameCommon.hpp"

#include "Game/Game.hpp"
#include "Game/GameState.hpp"
#include "Game/Player.hpp"
#include "Game/Ufo.hpp"

#include <memory>
#include <vector>

class Asteroid;
class Bullet;
class Explosion;
class Ship;
class Entity;
class Ufo;

class MainState : public GameState {
public:
    virtual ~MainState() = default;

    void OnEnter() noexcept override;
    void OnExit() noexcept override;
    void BeginFrame() noexcept override;
    void Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) override;
    void Render() const noexcept override;
    void EndFrame() noexcept override;

    mutable Ship* ship{nullptr};
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

    void MakeUfo() noexcept;
    void MakeUfo(Ufo::Type type) noexcept;

    void MakeShip() noexcept;
    void MakeLargeAsteroidAtMouse() noexcept;

    void Respawn() noexcept;

    void HandleBulletCollision() const noexcept;
    void HandleBulletAsteroidCollision() const noexcept;
    void HandleBulletUfoCollision() const noexcept;
    void HandleShipCollision() noexcept;
    void HandleShipAsteroidCollision() noexcept;
    void HandleShipBulletCollision() noexcept;
    void HandleMineCollision() noexcept;
    void HandleMineUfoCollision() noexcept;
    void HandleMineAsteroidCollision() noexcept;
    void KillAll() noexcept;

    unsigned int GetWaveMultiplierFromDifficulty() const noexcept;
    long long GetLivesFromDifficulty() const noexcept;

    void RenderBackground() const noexcept;
    void RenderEntities() const noexcept;
    void DebugRenderEntities() const noexcept;

    AABB2 CalculateCameraBounds() const noexcept;

    void RenderStatus() const noexcept;

    void DoCameraShake() noexcept;

    AABB2 world_bounds = AABB2::ZERO_TO_ONE;

    OrthographicCameraController m_cameraController{};
    Vector2 _auto_target_location{};
    float m_thrust_force{100.0f};
    bool m_debug_render{false};
};

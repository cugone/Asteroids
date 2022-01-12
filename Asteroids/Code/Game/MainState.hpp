#pragma once

#include "Engine/Core/TypeUtils.hpp"
#include "Engine/Core/OrthographicCameraController.hpp"
#include "Engine/Core/Stopwatch.hpp"

#include "Engine/Math/AABB2.hpp"

#include "Engine/Renderer/Camera2D.hpp"

#include "Engine/Scene/Scene.hpp"

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
class GameEntity;
class Ufo;
class Mine;

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
    
    void MakeExplosion(Vector2 position) noexcept;
    void MakeMediumAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept;
    void MakeSmallAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept;
    void MakeBullet(const GameEntity* parent, Vector2 pos, Vector2 vel) noexcept;
    void MakeMine(const GameEntity* parent, Vector2 position) noexcept;

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

    void WrapAroundWorld(GameEntity* e) noexcept;
    void UpdateEntities(TimeUtils::FPSeconds deltaSeconds) noexcept;
    void StartNewWave(unsigned int wave_number) noexcept;

    void MakeLargeAsteroidOffScreen(AABB2 world_bounds) noexcept;
    void MakeLargeAsteroidAt(Vector2 pos) noexcept;
    void MakeLargeAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept;

    void AddNewAsteroidToWorld(std::unique_ptr<Asteroid> newAsteroid);

    Ship* GetShip() const noexcept;

    void MakeSmallUfo(AABB2 world_bounds) noexcept;
    void MakeBigUfo(AABB2 world_bounds) noexcept;
    void MakeBossUfo(AABB2 world_bounds) noexcept;

    void AddNewUfoToWorld(std::unique_ptr<Ufo> newUfo) noexcept;

    void MakeUfo() noexcept;
    void MakeUfo(Ufo::Type type) noexcept;

    void MakeUfo(Ufo::Type type, AABB2 world_bounds) noexcept;

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
    void RenderFadeOutOverlay() const noexcept;
    void RenderPausedOverlay() const noexcept;

    AABB2 CalculateCameraBounds() const noexcept;

    void RenderStatus() const noexcept;

    void DoCameraShake() noexcept;
    bool DoFadeOut(TimeUtils::FPSeconds deltaSeconds) noexcept;

    void PostFrameCleanup() noexcept;

    AABB2 m_world_bounds = AABB2::Zero_to_One;

    std::shared_ptr<Scene> m_Scene{};
    unsigned int m_current_wave{1u};
    std::vector<Asteroid*> asteroids{};
    std::vector<Ufo*> ufos{};
    std::vector<Bullet*> bullets{};
    std::vector<Explosion*> explosions{};
    std::vector<Mine*> mines{};
    std::vector<std::unique_ptr<GameEntity>> m_entities{};
    std::vector<std::unique_ptr<GameEntity>> m_pending_entities{};

    OrthographicCameraController m_cameraController{};
    float m_thrust_force{100.0f};
    float m_fadeOut_alpha{0.0f};
    bool m_debug_render{false};
    bool IsWaveComplete() const noexcept;
};

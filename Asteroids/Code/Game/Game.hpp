#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/Vertex3D.hpp"
#include "Engine/Core/Stopwatch.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Vector4.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Math/Vector2.hpp"

#include "Engine/Renderer/Camera2D.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/Mesh.hpp"

#include "Game/Entity.hpp"
#include "Game/Player.hpp"

#include <memory>
#include <vector>

class Asteroid;
class Bullet;
class Explosion;
class Ship;


enum class GameState {
    Title,
    Main,
    GameOver,
};

class Game {
public:
    Game() = default;
    Game(const Game& other) = default;
    Game(Game&& other) = default;
    Game& operator=(const Game& other) = default;
    Game& operator=(Game&& other) = default;
    ~Game() = default;

    void Initialize();
    void BeginFrame();
    void Update(TimeUtils::FPSeconds deltaSeconds);
    void Render() const;
    void EndFrame();

    void DecrementLives() noexcept;

    void MakeExplosion(Vector2 position) noexcept;
    void MakeBullet(const Entity* parent, Vector2 pos, Vector2 vel) noexcept;
    void MakeMediumAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept;
    void MakeSmallAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept;

    Player player{};
    Ship* ship{nullptr};
    AABB2 world_bounds = AABB2::NEG_ONE_TO_ONE;
    std::vector<Asteroid*> asteroids{};
    std::vector<Bullet*> bullets{};
    std::vector<Explosion*> explosions{};
    std::shared_ptr<SpriteSheet> asteroid_sheet{};
    std::shared_ptr<SpriteSheet> explosion_sheet{};

protected:
private:
    void HandleDebugInput(Camera2D& baseCamera, TimeUtils::FPSeconds deltaSeconds);
    void HandleDebugKeyboardInput(Camera2D& baseCamera, TimeUtils::FPSeconds deltaSeconds);
    void HandleDebugMouseInput(Camera2D& baseCamera, TimeUtils::FPSeconds deltaSeconds);

    void HandlePlayerInput(Camera2D& baseCamera, TimeUtils::FPSeconds deltaSeconds);
    void HandleKeyboardInput(Camera2D& baseCamera, TimeUtils::FPSeconds deltaSeconds);
    void HandleMouseInput(Camera2D& baseCamera, TimeUtils::FPSeconds deltaSeconds);
    void HandleControllerInput(Camera2D& baseCamera, TimeUtils::FPSeconds deltaSeconds);

    void UpdateEntities(TimeUtils::FPSeconds deltaSeconds) noexcept;

    void MakeLargeAsteroidOffScreen() noexcept;
    void MakeLargeAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept;
    void MakeShip() noexcept;

    void KillAll() noexcept;
    void HandleBulletAsteroidCollision() const noexcept;
    void HandleShipAsteroidCollision() const noexcept;

    void ChangeState(GameState newState) noexcept;
    void OnEnterState(GameState state) noexcept;
    void OnExitState(GameState state) noexcept;

    void OnEnter_Title() noexcept;
    void OnEnter_Main() noexcept;
    void OnEnter_GameOver() noexcept;

    void OnExit_Title() noexcept;
    void OnExit_Main() noexcept;
    void OnExit_GameOver() noexcept;

    void BeginFrame_Title() noexcept;
    void BeginFrame_Main() noexcept;
    void BeginFrame_GameOver() noexcept;

    void Update_Title([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void Update_Main([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void Update_GameOver([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    
    void Render_Title() const noexcept;
    void Render_Main() const noexcept;
    void Render_GameOver() const noexcept;
    
    void EndFrame_Title() noexcept;
    void EndFrame_Main() noexcept;
    void EndFrame_GameOver() noexcept;

    void RenderBackground(const Vector2& ui_view_half_extents) const noexcept;
    void RenderEntities() const noexcept;
    void DebugRenderEntities() const noexcept;
    void RenderStatus(const Vector2 cameraPos, const  Vector2 viewHalfExtents) const noexcept;

    void StartNewWave(unsigned int wave_number) noexcept;
    void Respawn() noexcept;
    bool IsGameOver() const noexcept;

    void SetControlType() noexcept;

    mutable Camera2D _camera2D{};
    std::vector<std::unique_ptr<Entity>> _entities{};
    std::vector<std::unique_ptr<Entity>> _pending_entities{};
    float _thrust_force{100.0f};
    unsigned int _current_wave{1};
    GameState _current_state{GameState::Title};
    GameState _next_state{GameState::Title};
    bool _debug_render{false};
    bool _keyboard_control_active{false};
    bool _mouse_control_active{false};
    bool _controller_control_active{false};
};


#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/OrthographicCameraController.hpp"
#include "Engine/Core/Vertex3D.hpp"
#include "Engine/Core/Stopwatch.hpp"

#include "Engine/Input/InputSystem.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Vector4.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Math/Vector2.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Camera2D.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/Mesh.hpp"

#include "Game/GameCommon.hpp"

#include "Game/Entity.hpp"
#include "Game/Player.hpp"

#include <memory>
#include <vector>

class Asteroid;
class Bullet;
class Explosion;
class Ship;

enum class Difficulty {
    Easy,
    Normal,
    Hard
};

struct GameOptions {
    Difficulty difficulty{Difficulty::Normal};
};

enum class GameState {
    Title,
    Options,
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

    const GameOptions& GetGameOptions() const noexcept;

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
    long long GetLivesFromDifficulty() const noexcept;

    void HandleDebugInput(TimeUtils::FPSeconds deltaSeconds);
    void HandleDebugKeyboardInput(TimeUtils::FPSeconds deltaSeconds);
    void HandleDebugMouseInput(TimeUtils::FPSeconds deltaSeconds);

    void HandlePlayerInput(TimeUtils::FPSeconds deltaSeconds);
    void HandleKeyboardInput(TimeUtils::FPSeconds deltaSeconds);
    void HandleMouseInput(TimeUtils::FPSeconds deltaSeconds);
    void HandleControllerInput(TimeUtils::FPSeconds deltaSeconds);

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
    void OnEnter_Options() noexcept;
    void OnEnter_Main() noexcept;
    void OnEnter_GameOver() noexcept;

    void OnExit_Title() noexcept;
    void OnExit_Options() noexcept;
    void OnExit_Main() noexcept;
    void OnExit_GameOver() noexcept;

    void BeginFrame_Title() noexcept;
    void BeginFrame_Options() noexcept;
    void BeginFrame_Main() noexcept;
    void BeginFrame_GameOver() noexcept;

    void Update_Title([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void Update_Options([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void Update_Main([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void Update_GameOver([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    
    void Render_Title() const noexcept;
    void Render_Options() const noexcept;
    void Render_Main() const noexcept;
    void Render_GameOver() const noexcept;
    
    void EndFrame_Title() noexcept;
    void EndFrame_Options() noexcept;
    void EndFrame_Main() noexcept;
    void EndFrame_GameOver() noexcept;

    void RenderBackground() const noexcept;
    void RenderEntities() const noexcept;
    void DebugRenderEntities() const noexcept;
    void RenderStatus() const noexcept;

    void StartNewWave(unsigned int wave_number) noexcept;
    void Respawn() noexcept;
    bool IsGameOver() const noexcept;

    void SetControlType() noexcept;

    mutable Camera2D _ui_camera{};
    OrthographicCameraController _cameraController{};
    std::vector<std::unique_ptr<Entity>> _entities{};
    std::vector<std::unique_ptr<Entity>> _pending_entities{};
    float _thrust_force{100.0f};
    unsigned int _current_wave{1};
    GameOptions _current_options{};
    GameState _current_state{GameState::Title};
    GameState _next_state{GameState::Title};
    bool _debug_render{false};
    bool _keyboard_control_active{false};
    bool _mouse_control_active{false};
    bool _controller_control_active{false};
    bool _controlling_camera{false};
};


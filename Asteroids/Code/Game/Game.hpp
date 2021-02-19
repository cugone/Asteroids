#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/TypeUtils.hpp"
#include "Engine/Core/OrthographicCameraController.hpp"
#include "Engine/Core/Vertex3D.hpp"

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

#include "Game/GameState.hpp"
#include "Game/Entity.hpp"
#include "Game/Player.hpp"
#include "Game/Ufo.hpp"

#include <memory>
#include <string>
#include <vector>

class Asteroid;
class Bullet;
class Explosion;
class Ship;

enum class Difficulty {
    First_,
    Easy = First_,
    Normal,
    Hard,
    Last_,
};

template<>
struct TypeUtils::is_incrementable_enum_type<Difficulty> : std::true_type {};

template<>
struct TypeUtils::is_decrementable_enum_type<Difficulty> : std::true_type {};

enum class ControlPreference {
    First_,
    Keyboard = First_,
    Mouse,
    XboxController,
    Last_,
};

template<>
struct TypeUtils::is_incrementable_enum_type<ControlPreference> : std::true_type {};

template<>
struct TypeUtils::is_decrementable_enum_type<ControlPreference> : std::true_type {};

struct GameOptions {
    Difficulty difficulty{Difficulty::Normal};
    ControlPreference controlPref{ControlPreference::Mouse};
    uint8_t soundVolume{5};
    uint8_t musicVolume{5};
    float cameraShakeStrength{1.0f};
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
    void PostFrameCleanup() noexcept;

    std::vector<std::unique_ptr<Entity>>& GetEntities() noexcept;

    Ship* GetShip() const noexcept;

    void MakeExplosion(Vector2 position) noexcept;
    void MakeBullet(const Entity* parent, Vector2 pos, Vector2 vel) noexcept;

    void MakeSmallUfo(AABB2 world_bounds) noexcept;
    void MakeBigUfo(AABB2 world_bounds) noexcept;
    void MakeBossUfo(AABB2 world_bounds) noexcept;

    void AddNewUfoToWorld(std::unique_ptr<Ufo> newUfo) noexcept;
    void AddNewAsteroidToWorld(std::unique_ptr<Asteroid> newAsteroid);
    void MakeLargeAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept;
    void MakeLargeAsteroidAt(Vector2 pos) noexcept;
    void MakeLargeAsteroidOffScreen(AABB2 world_bounds) noexcept;
    void MakeMediumAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept;
    void MakeSmallAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept;

    void DoCameraShake(OrthographicCameraController& cameraController) const noexcept;

    void SetControlType() noexcept;
    bool IsControllerActive() const noexcept;
    bool IsKeyboardActive() const noexcept;
    bool IsMouseActive() const noexcept;

    void ChangeState(std::unique_ptr<GameState> newState) noexcept;
    void DecrementLives() noexcept;
    bool IsGameOver() const noexcept;
    void SetAsteroidSpriteSheet() noexcept;
    void SetExplosionSpriteSheet() noexcept;
    void SetUfoSpriteSheets() noexcept;

    AABB2 CalcOrthoBounds(const OrthographicCameraController& cameraController) const noexcept;
    AABB2 CalcViewBounds(const OrthographicCameraController& cameraController) const noexcept;
    AABB2 CalcCullBounds(const OrthographicCameraController& cameraController) const noexcept;
    AABB2 CalcCullBoundsFromOrthoBounds(const OrthographicCameraController& cameraController) const noexcept;

    GameOptions gameOptions{};
    Player player{};
    unsigned int m_current_wave{1u};
    std::vector<Asteroid*> asteroids{};
    std::vector<Ufo*> ufos{};
    std::vector<Bullet*> bullets{};
    std::vector<Explosion*> explosions{};
    std::shared_ptr<SpriteSheet> asteroid_sheet{};
    std::shared_ptr<SpriteSheet> explosion_sheet{};
    std::shared_ptr<SpriteSheet> ufo_sheet{};

    Stopwatch respawnTimer{TimeUtils::FPSeconds{1.0f}};
    bool easyMode{false};

protected:
private:
    void InitializeAudio() noexcept;
    void InitializeSounds() noexcept;
    void InitializeMusic() noexcept;

    void CreateOrLoadOptionsFile() noexcept;
    void CreateOptionsFile() const noexcept;
    void LoadOptionsFile() const noexcept;

    void MakeUfo(Ufo::Type type, AABB2 world_bounds) noexcept;

    std::unique_ptr<GameState> _current_state{nullptr};
    std::unique_ptr<GameState> _next_state{nullptr};
    std::vector<std::unique_ptr<Entity>> m_entities{};
    std::vector<std::unique_ptr<Entity>> m_pending_entities{};
    bool _keyboard_control_active{false};
    bool _mouse_control_active{false};
    bool _controller_control_active{false};
    bool _controlling_camera{false};
};


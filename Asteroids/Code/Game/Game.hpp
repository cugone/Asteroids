#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/TypeUtils.hpp"
#include "Engine/Core/OrthographicCameraController.hpp"

#include "Engine/Game/GameBase.hpp"
#include "Engine/Game/GameSettings.hpp"

#include "Engine/Renderer/Vertex3D.hpp"

#include "Engine/Input/InputSystem.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Vector4.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Math/Vector2.hpp"

#include "Engine/Physics/Particles/ParticleSystem.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Camera2D.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/Mesh.hpp"

#include "Game/GameCommon.hpp"

#include "Game/GameState.hpp"
#include "Game/GameEntity.hpp"
#include "Game/Player.hpp"
#include "Game/Ufo.hpp"

#include <memory>
#include <string>
#include <vector>

class Asteroid;
class Bullet;
class Explosion;
class Ship;
class Mine;

enum class Difficulty {
    First_,
    Easy = First_,
    Normal,
    Last_Valid_,
    Hard = Last_Valid_,
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
    Last_Valid_,
    XboxController = Last_Valid_,
    Last_,
};

template<>
struct TypeUtils::is_incrementable_enum_type<ControlPreference> : std::true_type {};

template<>
struct TypeUtils::is_decrementable_enum_type<ControlPreference> : std::true_type {};

class GameOptions : public GameSettings {
public:
    GameOptions() noexcept = default;
    GameOptions(const GameOptions& other) noexcept = default;
    GameOptions(GameOptions&& other) noexcept = default;
    virtual ~GameOptions() noexcept = default;
    GameOptions& operator=(const GameOptions& rhs) noexcept = default;
    GameOptions& operator=(GameOptions&& rhs) noexcept = default;

    virtual void SaveToConfig(Config& config) noexcept override;
    virtual void SetToDefault() noexcept override;

    void SetDifficulty(const Difficulty& newDifficulty) noexcept;
    Difficulty GetDifficulty() const noexcept;
    Difficulty DefaultDifficulty() const noexcept;

    void SetControlPreference(const ControlPreference& newControlPreference) noexcept;
    ControlPreference GetControlPreference() const noexcept;
    ControlPreference DefaultControlPreference() const noexcept;

    void SetSoundVolume(uint8_t newSoundVolume) noexcept;
    uint8_t GetSoundVolume() const noexcept;
    uint8_t DefaultSoundVolume() const noexcept;
    
    void SetMusicVolume(uint8_t newMusicVolume) noexcept;
    uint8_t GetMusicVolume() const noexcept;
    uint8_t DefaultMusicVolume() const noexcept;

    void SetCameraShakeStrength(float newCameraShakeStrength) noexcept;
    float GetCameraShakeStrength() const noexcept;
    float DefaultCameraShakeStrength() const noexcept;

    float GetMaxShakeOffsetHorizontal() const noexcept;
    float GetMaxShakeOffsetVertical() const noexcept;
    float GetMaxShakeAngle() const noexcept;

protected:

    Difficulty _difficulty{Difficulty::Normal};
    Difficulty _defaultDifficulty{Difficulty::Normal};
    ControlPreference _controlPref{ControlPreference::Mouse};
    ControlPreference _defaultControlPref{ControlPreference::Mouse};
    uint8_t _soundVolume{5};
    uint8_t _defaultSoundVolume{5};
    uint8_t _musicVolume{5};
    uint8_t _defaultMusicVolume{5};
    float _cameraShakeStrength{1.0f};
    float _defaultCameraShakeStrength{1.0f};

private:
    float _maxShakeOffsetHorizontal{50.0f};
    float _maxShakeOffsetVertical{50.0f};
    float _maxShakeAngle{10.0f};
};

class Game : public GameBase {
public:
    Game() = default;
    Game(const Game& other) = default;
    Game(Game&& other) = default;
    Game& operator=(const Game& other) = default;
    Game& operator=(Game&& other) = default;
    ~Game() noexcept = default;

    void Initialize() noexcept override;
    void BeginFrame() noexcept override;
    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void Render() const noexcept override;
    void EndFrame() noexcept override;

    std::vector<std::unique_ptr<GameEntity>>& GetEntities() noexcept;

    void DoCameraShake(OrthographicCameraController& controller) const noexcept;

    void SetControlType() noexcept;
    bool IsControllerActive() const noexcept;
    bool IsKeyboardActive() const noexcept;
    bool IsMouseActive() const noexcept;

    void ChangeState(std::unique_ptr<GameState> newState) noexcept;
    void DecrementLives() noexcept;
    bool IsGameOver() const noexcept;
    void TogglePause() noexcept;
    bool IsPaused() const noexcept;

    void SetAsteroidSpriteSheet() noexcept;
    void SetMineSpriteSheet() noexcept;
    void SetExplosionSpriteSheet() noexcept;
    void SetUfoSpriteSheets() noexcept;
    void SetLaserChargeSpriteSheet() noexcept;

    AABB2 CalcOrthoBounds(const OrthographicCameraController& controller) const noexcept;
    AABB2 CalcViewBounds(const OrthographicCameraController& controller) const noexcept;
    AABB2 CalcCullBounds(const OrthographicCameraController& controller) const noexcept;
    AABB2 CalcCullBoundsFromOrthoBounds(const OrthographicCameraController& controller) const noexcept;

    GameOptions gameOptions{};
    Player player{};

    std::shared_ptr<SpriteSheet> asteroid_sheet{};
    std::shared_ptr<SpriteSheet> lasercharge_sheet{};
    std::shared_ptr<SpriteSheet> mine_sheet{};
    std::shared_ptr<SpriteSheet> explosion_sheet{};
    std::shared_ptr<SpriteSheet> ufo_sheet{};

    Stopwatch respawnTimer{TimeUtils::FPSeconds{1.0f}};
    std::unique_ptr<ParticleSystem> particleSystem{};

    GameState* const GetCurrentState() const noexcept;
protected:
private:
    void InitializeAudio() noexcept;
    void InitializeSounds() noexcept;
    void InitializeMusic() noexcept;

    void CreateOrLoadOptionsFile() noexcept;
    void CreateOptionsFile() const noexcept;
    void LoadOptionsFile() const noexcept;

    std::unique_ptr<GameState> _current_state{nullptr};
    std::unique_ptr<GameState> _next_state{nullptr};
    bool _keyboard_control_active{false};
    bool _mouse_control_active{false};
    bool _controller_control_active{false};
    bool _controlling_camera{false};
    bool _paused{false};
};


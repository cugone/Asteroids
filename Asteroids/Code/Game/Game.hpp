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

    void MakeBullet(const Entity* parent, Vector2 pos, Vector2 vel) noexcept;
    void MakeLargeAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept;
    void MakeMediumAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept;
    void MakeSmallAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept;
    void MakeShip() noexcept;
    void MakeExplosion(Vector2 position) noexcept;

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

    void HandleBulletAsteroidCollision() const noexcept;
    void HandleShipAsteroidCollision() const noexcept;

    void RenderEntities() const noexcept;
    void DebugRenderEntities() const noexcept;
    void RenderStatus(const Vector2 cameraPos, const  Vector2 viewHalfExtents) const noexcept;

    mutable Camera2D _camera2D{};
    std::vector<std::unique_ptr<Entity>> _entities{};
    std::vector<std::unique_ptr<Entity>> _pending_entities{};
    float _thrust_force{1.0f};
    bool _debug_render{false};
    bool _keyboard_control_active{false};
    bool _mouse_control_active{false};
    bool _controller_control_active{false};
};


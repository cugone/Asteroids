#include "Game/Game.hpp"

#include "Engine/Core/KerningFont.hpp"

#include "Engine/Math/Disc2.hpp"

#include "Engine/Renderer/Material.hpp"

#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"

#include "Game/Entity.hpp"
#include "Game/Asteroid.hpp"
#include "Game/Bullet.hpp"
#include "Game/Explosion.hpp"
#include "Game/Ship.hpp"

#include <algorithm>

void Game::Initialize() {
    g_theRenderer->SetWindowTitle("Asteroids");
    g_theAudioSystem->RegisterWavFilesFromFolder("Data/Audio/");
    g_theRenderer->RegisterMaterialsFromFolder("Data/Materials/");
    world_bounds = AABB2{Vector2::ZERO, Vector2{g_theRenderer->GetOutput()->GetDimensions()}};
    _entities.emplace_back(std::move(std::make_unique<Ship>(world_bounds.CalcCenter())));
    ship = reinterpret_cast<Ship*>(_entities.back().get());
}

void Game::BeginFrame() {
    for(auto& entity : _entities) {
        if(entity) {
            entity->BeginFrame();
        }
    }
}

void Game::Update(TimeUtils::FPSeconds deltaSeconds) {
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::Esc)) {
        g_theApp->SetIsQuitting(true);
        return;
    }
    if(GameOver()) {
        return;
    }
    Camera2D& base_camera = _camera2D;
    HandleDebugInput(base_camera, deltaSeconds);
    HandlePlayerInput(base_camera, deltaSeconds);
    base_camera.Update(deltaSeconds);

    UpdateEntities(deltaSeconds);
}

void Game::Render() const {
    g_theRenderer->ResetModelViewProjection();
    g_theRenderer->SetRenderTargetsToBackBuffer();
    g_theRenderer->ClearDepthStencilBuffer();

    g_theRenderer->ClearColor(Rgba::Black);

    g_theRenderer->SetViewportAsPercent();

    //2D View / HUD
    const float ui_view_height = currentGraphicsOptions.WindowHeight;
    const float ui_view_width = ui_view_height * _camera2D.GetAspectRatio();
    const auto ui_view_extents = Vector2{ui_view_width, ui_view_height};
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    auto ui_leftBottom = Vector2{-ui_view_half_extents.x, ui_view_half_extents.y};
    auto ui_rightTop = Vector2{ui_view_half_extents.x, -ui_view_half_extents.y};
    auto ui_nearFar = Vector2{0.0f, 1.0f};
    auto ui_cam_pos = ui_view_half_extents;
    _camera2D.position = ui_cam_pos;
    _camera2D.orientation_degrees = 0.0f;
    _camera2D.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, MathUtils::M_16_BY_9_RATIO);
    g_theRenderer->SetCamera(_camera2D);

    if(!GameOver()) {
        RenderEntities();
        if(_debug_render) {
            DebugRenderEntities();
        }
        RenderStatus(ui_cam_pos, ui_view_half_extents);
    } else {
        const auto* font = g_theRenderer->GetFont("System32");
        g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(ui_view_half_extents));
        g_theRenderer->DrawTextLine(font, "GAME OVER");
    }
}

void Game::RenderStatus(const Vector2 camPos, const Vector2 viewHalfExtents) const noexcept {
    g_theRenderer->SetModelMatrix(Matrix4::I);
    const auto* font = g_theRenderer->GetFont("System32");
    const auto font_position = camPos - viewHalfExtents + Vector2{5.0f, font->GetLineHeight() * 0.0f};
    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(font_position));
    g_theRenderer->DrawMultilineText(g_theRenderer->GetFont("System32"), "Score: " + std::to_string(player.GetScore()) + "\n      x" + std::to_string(player.lives));

    const auto uvs = AABB2::ZERO_TO_ONE;
    const auto mat = g_theRenderer->GetMaterial("ship");
    const auto tex = mat->GetTexture(Material::TextureID::Diffuse);
    const auto frameWidth = static_cast<float>(tex->GetDimensions().x);
    const auto frameHeight = static_cast<float>(tex->GetDimensions().y);
    const auto half_extents = Vector2{frameWidth, frameHeight};
    const auto S = Matrix4::CreateScaleMatrix(half_extents);
    const auto R = Matrix4::I;
    const auto T = Matrix4::CreateTranslationMatrix(font_position + Vector2{15.0f + font->CalculateTextWidth(" "), font->GetLineHeight() * 1.8f});
    const auto transform = Matrix4::MakeSRT(S, R, T);

    g_theRenderer->SetModelMatrix(transform);
    g_theRenderer->SetMaterial(mat);
    g_theRenderer->DrawQuad2D();

}

void Game::Respawn() noexcept {
    MakeShip();
}

bool Game::GameOver() const noexcept {
    return player.lives == 0;
}

void Game::DecrementLives() noexcept {
    player.DecrementLives();
}

void Game::EndFrame() {
    for(auto& entity : _entities) {
        if(entity) {
            entity->EndFrame();
        }
    }
    for(auto& entity : _entities) {
        if(entity && entity->IsDead()) {
            entity->OnDestroy();
            entity.reset();
        }
    }
    bullets.erase(std::remove_if(std::begin(bullets), std::end(bullets), [&](Bullet* e) { return !e; }), std::end(bullets));
    asteroids.erase(std::remove_if(std::begin(asteroids), std::end(asteroids), [&](Asteroid* e) { return !e; }), std::end(asteroids));
    _entities.erase(std::remove_if(std::begin(_entities) + 1, std::end(_entities), [&](std::unique_ptr<Entity>& e) { return !e; }), std::end(_entities));

    for(auto&& pending : _pending_entities) {
        _entities.emplace_back(std::move(pending));
    }
    _pending_entities.clear();
}

void Game::MakeBullet(const Entity* parent, Vector2 pos, Vector2 vel) noexcept {
    auto newBullet = std::make_unique<Bullet>(parent, pos, vel);
    auto* last_entity = newBullet.get();
    _pending_entities.emplace_back(std::move(newBullet));
    auto* asBullet = reinterpret_cast<Bullet*>(last_entity);
    bullets.push_back(asBullet);
    asBullet->OnCreate();
}

void Game::MakeLargeAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept {
    if(!asteroid_sheet) {
        asteroid_sheet = g_theRenderer->CreateSpriteSheet("Data/Images/asteroid.png", 6, 5);
    }
    auto newAsteroid = std::make_unique<Asteroid>(Asteroid::Type::Large, pos, vel, rotationSpeed);
    auto* last_entity = newAsteroid.get();
    _pending_entities.emplace_back(std::move(newAsteroid));
    auto* asAsteroid = reinterpret_cast<Asteroid*>(last_entity);
    asteroids.push_back(asAsteroid);
}

void Game::MakeMediumAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept {
    if(!asteroid_sheet) {
        asteroid_sheet = g_theRenderer->CreateSpriteSheet("Data/Images/asteroid.png", 6, 5);
    }
    auto newAsteroid = std::make_unique<Asteroid>(Asteroid::Type::Medium, pos, vel, rotationSpeed);
    auto* last_entity = newAsteroid.get();
    _pending_entities.emplace_back(std::move(newAsteroid));
    auto* asAsteroid = reinterpret_cast<Asteroid*>(last_entity);
    asteroids.push_back(asAsteroid);
}

void Game::MakeSmallAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept {
    if(!asteroid_sheet) {
        asteroid_sheet = g_theRenderer->CreateSpriteSheet("Data/Images/asteroid.png", 6, 5);
    }
    auto newAsteroid = std::make_unique<Asteroid>(Asteroid::Type::Small, pos, vel, rotationSpeed);
    auto* last_entity = newAsteroid.get();
    _pending_entities.emplace_back(std::move(newAsteroid));
    auto* asAsteroid = reinterpret_cast<Asteroid*>(last_entity);
    asteroids.push_back(asAsteroid);
}

void Game::MakeShip() noexcept {
    if(!ship) {
        auto iter = _entities.begin();
        *iter = std::move(std::make_unique<Ship>(world_bounds.CalcCenter()));
        ship = reinterpret_cast<Ship*>(iter->get());
    }
}

void Game::MakeExplosion(Vector2 position) noexcept {
    if(!explosion_sheet) {
        explosion_sheet = g_theRenderer->CreateSpriteSheet("Data/Images/explosion.png", 5, 5);
    }
    auto newExplosion = std::make_unique<Explosion>(position);
    auto* last_entity = newExplosion.get();
    _pending_entities.emplace_back(std::move(newExplosion));
    auto* asExplosion = reinterpret_cast<Explosion*>(last_entity);
    explosions.push_back(asExplosion);
    asExplosion->OnCreate();
}

void Game::HandlePlayerInput(Camera2D& baseCamera, TimeUtils::FPSeconds deltaSeconds) {
    HandleKeyboardInput(baseCamera, deltaSeconds);
    HandleMouseInput(baseCamera, deltaSeconds);
    HandleControllerInput(baseCamera, deltaSeconds);
}

void Game::HandleKeyboardInput(Camera2D& /*baseCamera*/, TimeUtils::FPSeconds deltaSeconds) {
    if(g_theInputSystem->WasAnyKeyPressed()) {
        _keyboard_control_active = true;
        _mouse_control_active = false;
        _controller_control_active = false;
    }
    if(!_keyboard_control_active) {
        return;
    }
    if(ship) {
        if(g_theInputSystem->IsKeyDown(KeyCode::A)) {
            ship->RotateCounterClockwise(ship->GetRotationSpeed() * deltaSeconds.count());
        } else if(g_theInputSystem->IsKeyDown(KeyCode::D)) {
            ship->RotateClockwise(ship->GetRotationSpeed() * deltaSeconds.count());
        }
        if(g_theInputSystem->IsKeyDown(KeyCode::W)) {
            ship->Thrust(_thrust_force);
        }
        if(g_theInputSystem->WasKeyJustPressed(KeyCode::Space)) {
            ship->OnFire();
        }
    }
}

void Game::HandleMouseInput(Camera2D& baseCamera, TimeUtils::FPSeconds /*deltaSeconds*/) {
    if(g_theInputSystem->WasMouseJustUsed()) {
        _keyboard_control_active = false;
        _mouse_control_active = true;
        _controller_control_active = false;
    }
    if(!_mouse_control_active) {
        return;
    }
    if(ship) {
        if(g_theInputSystem->GetMouseDelta().CalcLengthSquared() > 0.0f) {
            auto mouseWorldCoords = g_theRenderer->ConvertScreenToWorldCoords(baseCamera, g_theInputSystem->GetCursorScreenPosition());
            const auto newFacing = (mouseWorldCoords - ship->GetPosition()).CalcHeadingDegrees();
            ship->SetOrientationDegrees(newFacing);
        }

        if(g_theInputSystem->WasKeyJustPressed(KeyCode::LButton)) {
            ship->OnFire();
        }
        if(g_theInputSystem->IsKeyDown(KeyCode::RButton)) {
            ship->Thrust(_thrust_force);
        }
    }
}

void Game::HandleControllerInput(Camera2D& /*baseCamera*/, TimeUtils::FPSeconds /*deltaSeconds*/) {
    if(g_theInputSystem->WasAnyControllerJustUsed()) {
        _keyboard_control_active = false;
        _mouse_control_active = false;
        _controller_control_active = true;
    }
    if(!_controller_control_active) {
        return;
    }
    if(ship) {
        if(auto& controller = g_theInputSystem->GetXboxController(0); controller.IsConnected() && controller.IsButtonDown(XboxController::Button::A)) {
            ship->Thrust(_thrust_force);
        }
        if(auto& controller = g_theInputSystem->GetXboxController(0); controller.IsConnected() && controller.GetRightTriggerPosition() > 0.0f) {
            ship->OnFire();
        }
        if(auto& controller = g_theInputSystem->GetXboxController(0); controller.IsConnected() && controller.GetLeftThumbPosition().CalcLengthSquared() > 0.0f) {
            const auto newFacing = controller.GetLeftThumbPosition().CalcHeadingDegrees();
            ship->SetOrientationDegrees(newFacing);
        }
    }
}

void Game::UpdateEntities(TimeUtils::FPSeconds deltaSeconds) noexcept {
    for(auto& entity : _entities) {
        if(entity) {
            entity->Update(deltaSeconds);
        }
    }
    HandleBulletAsteroidCollision();
    HandleShipAsteroidCollision();
}

void Game::HandleBulletAsteroidCollision() const noexcept {
    for(auto& bullet : bullets) {
        for(auto& asteroid : asteroids) {
            Disc2 bulletCollisionMesh{bullet->GetPosition(), bullet->GetPhysicalRadius()};
            Disc2 asteroidCollisionMesh{asteroid->GetPosition(), asteroid->GetPhysicalRadius()};
            if(MathUtils::DoDiscsOverlap(bulletCollisionMesh, asteroidCollisionMesh)) {
                bullet->OnCollision(bullet, asteroid);
                asteroid->OnCollision(asteroid, bullet);
            }
        }
    }
}

void Game::HandleShipAsteroidCollision() const noexcept {
    if(!ship) {
        return;
    }
    for(auto& asteroid : asteroids) {
        Disc2 shipCollisionMesh{ship->GetPosition(), ship->GetPhysicalRadius()};
        Disc2 asteroidCollisionMesh{asteroid->GetPosition(), asteroid->GetPhysicalRadius()};
        if(MathUtils::DoDiscsOverlap(shipCollisionMesh, asteroidCollisionMesh)) {
            ship->OnCollision(ship, asteroid);
            asteroid->OnCollision(asteroid, ship);
        }
    }
}

void Game::RenderEntities() const noexcept {
    for(const auto& entity : _entities) {
        if(entity) {
            entity->Render(*g_theRenderer);
        }
    }
}

void Game::DebugRenderEntities() const noexcept {
    g_theRenderer->SetModelMatrix(Matrix4::I);
    g_theRenderer->SetMaterial("__2D");
    for(const auto& e : _entities) {
        if(!e) {
            continue;
        }
        const auto* entity = e.get();
        const auto center = entity->GetPosition();
        const auto orientation = entity->GetOrientationDegrees();
        const auto cosmetic_radius = entity->GetCosmeticRadius();
        const auto physical_radius = entity->GetPhysicalRadius();
        const auto facing_end = [=]()->Vector2 { auto end = Vector2::X_AXIS; end.SetLengthAndHeadingDegrees(orientation, cosmetic_radius); return center + end; }();
        const auto velocity_end = [=]()->Vector2 { auto end = entity->GetVelocity().GetNormalize(); end.SetLengthAndHeadingDegrees(end.CalcHeadingDegrees(), cosmetic_radius); return center + end; }();
        const auto acceleration_end = [=]()->Vector2 { auto end = entity->GetAcceleration().GetNormalize(); end.SetLengthAndHeadingDegrees(end.CalcHeadingDegrees(), cosmetic_radius); return center + end; }();
        g_theRenderer->DrawCircle2D(center, cosmetic_radius, Rgba::Green);
        g_theRenderer->DrawCircle2D(center, physical_radius, Rgba::Red);
        g_theRenderer->DrawLine2D(center, facing_end, Rgba::Red);
        g_theRenderer->DrawLine2D(center, velocity_end, Rgba::Green);
        g_theRenderer->DrawLine2D(center, acceleration_end, Rgba::Orange);
    }
}

void Game::HandleDebugInput(Camera2D& baseCamera, TimeUtils::FPSeconds deltaSeconds) {
    HandleDebugKeyboardInput(baseCamera, deltaSeconds);
    HandleDebugMouseInput(baseCamera, deltaSeconds);
}

void Game::HandleDebugKeyboardInput(Camera2D& baseCamera, TimeUtils::FPSeconds /*deltaSeconds*/) {
    if(g_theUISystem->GetIO().WantCaptureKeyboard) {
        return;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F1)) {
        _debug_render = !_debug_render;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F4)) {
        g_theUISystem->ToggleImguiDemoWindow();
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::I)) {
        const auto mouseWorldCoords = g_theRenderer->ConvertScreenToWorldCoords(baseCamera, g_theInputSystem->GetCursorScreenPosition());
        const auto vx = MathUtils::GetRandomFloatNegOneToOne();
        const auto vy = MathUtils::GetRandomFloatNegOneToOne();
        const auto s = MathUtils::GetRandomFloatInRange(20.0f, 100.0f);
        const auto vel = Vector2{vx, vy};
        const auto rot = MathUtils::GetRandomFloatNegOneToOne() * 180.0f;
        MakeLargeAsteroid(mouseWorldCoords, vel, rot);
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::N)) {
        MakeShip();
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::R)) {
        Respawn();
    }
}

void Game::HandleDebugMouseInput(Camera2D& /*baseCamera*/, TimeUtils::FPSeconds /*deltaSeconds*/) {
    if(g_theUISystem->GetIO().WantCaptureMouse) {
        return;
    }
}

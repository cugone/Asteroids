#include "Game/MainState.hpp"

#include "Engine/Core/KerningFont.hpp"
#include "Engine/Core/Utilities.hpp"

#include "Engine/Input/InputSystem.hpp"

#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/Matrix4.hpp"
#include "Engine/Math/MathUtils.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"

#include "Engine/UI/UISystem.hpp"

#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"
#include "Game/Ship.hpp"
#include "Game/Asteroid.hpp"
#include "Game/Bullet.hpp"
#include "Game/Explosion.hpp"
#include "Game/Mine.hpp"

#include "Game/TitleState.hpp"
#include "Game/GameOverState.hpp"

void MainState::OnEnter() noexcept {

    world_bounds = AABB2::ZERO_TO_ONE;
    auto dims = Vector2{g_theRenderer->GetOutput()->GetDimensions()};
    //TODO: Fix world dims
    world_bounds.ScalePadding(dims.x, dims.y);
    world_bounds.Translate(-world_bounds.CalcCenter());

    m_cameraController = OrthographicCameraController{g_theRenderer, g_theInputSystem};
    m_cameraController.SetPosition(world_bounds.CalcCenter());
    m_cameraController.SetZoomLevelRange(Vector2{225.0f, 450.0f});
    m_cameraController.SetZoomLevel(450.0f);

    PlayerDesc playerDesc{};
    playerDesc.lives = GetLivesFromDifficulty();
    g_theGame->player = Player{playerDesc};
    MakeShip();
}

void MainState::OnExit() noexcept {
    g_theGame->asteroids.clear();
    g_theGame->asteroids.shrink_to_fit();
    g_theGame->bullets.clear();
    g_theGame->bullets.shrink_to_fit();
    g_theGame->explosions.clear();
    g_theGame->explosions.shrink_to_fit();
    g_theGame->ufos.clear();
    g_theGame->ufos.shrink_to_fit();
    g_theGame->GetEntities().clear();
    g_theGame->GetEntities().shrink_to_fit();
    g_theGame->m_current_wave = 1u;
    ship = nullptr;
}

void MainState::BeginFrame() noexcept {
    g_theGame->SetControlType();
    for(auto& entity : g_theGame->GetEntities()) {
        if(entity) {
            entity->BeginFrame();
        }
    }
    if(!ship) {
        if(g_theGame->respawnTimer.CheckAndReset()) {
            Respawn();
        }
    }
}

void MainState::Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) {
    g_theRenderer->UpdateGameTime(deltaSeconds);
    HandleDebugInput(deltaSeconds);
    HandlePlayerInput(deltaSeconds);
    UpdateEntities(deltaSeconds);

    if(g_theGame->IsGameOver()) {
        g_theGame->ChangeState(std::move(std::make_unique<GameOverState>()));
    }

    m_cameraController.Update(deltaSeconds);
}

void MainState::Render() const noexcept {
    g_theRenderer->SetRenderTargetsToBackBuffer();
    g_theRenderer->ClearDepthStencilBuffer();

    g_theRenderer->ClearColor(Rgba::Black);

    g_theRenderer->SetViewportAsPercent();

    RenderBackground();
    RenderEntities();
    if(m_debug_render) {
        DebugRenderEntities();
    }
    RenderStatus();
}

void MainState::EndFrame() noexcept {
    for(auto& entity : g_theGame->GetEntities()) {
        if(entity) {
            entity->EndFrame();
        }
    }
    for(auto& entity : g_theGame->GetEntities()) {
        if(entity && entity->IsDead()) {
            entity->OnDestroy();
            entity.reset();
        }
    }
    g_theGame->PostFrameCleanup();
}

std::unique_ptr<GameState> MainState::HandleInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    return{};
}

std::unique_ptr<GameState> MainState::HandleKeyboardInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::Esc)) {
        return std::make_unique<TitleState>();
    }
    if(!g_theGame->IsKeyboardActive()) {
        return {};
    }
    if(!ship) {
        return {};
    }
    if(g_theInputSystem->IsKeyDown(KeyCode::A)) {
        ship->RotateClockwise(ship->GetRotationSpeed() * deltaSeconds.count());
    } else if(g_theInputSystem->IsKeyDown(KeyCode::D)) {
        ship->RotateCounterClockwise(ship->GetRotationSpeed() * deltaSeconds.count());
    }
    if(g_theInputSystem->IsKeyDown(KeyCode::W)) {
        ship->Thrust(m_thrust_force);
    } else {
        ship->Thrust(0.0f);
    }
    if(g_theInputSystem->IsKeyDown(KeyCode::Space)) {
        ship->OnFire();
    }
    if(g_theInputSystem->IsKeyDown(KeyCode::S)) {
        ship->DropMine();
    }
    return{};
}

std::unique_ptr<GameState> MainState::HandleControllerInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(!g_theGame->IsControllerActive()) {
        return {};
    }
    if(!ship) {
        return {};
    }
    if(auto& controller = g_theInputSystem->GetXboxController(0); controller.IsConnected() && controller.IsButtonDown(XboxController::Button::A)) {
        ship->Thrust(m_thrust_force);
    } else {
        ship->Thrust(0.0f);
    }
    if(auto& controller = g_theInputSystem->GetXboxController(0); controller.IsConnected() && controller.GetRightTriggerPosition() > 0.0f) {
        ship->OnFire();
    }
    if(auto& controller = g_theInputSystem->GetXboxController(0); controller.IsConnected() && controller.IsButtonDown(XboxController::Button::Y)) {
        ship->DropMine();
    }
    if(auto& controller = g_theInputSystem->GetXboxController(0); controller.IsConnected() && controller.GetLeftThumbPosition().CalcLengthSquared() > 0.0f) {
        const auto newFacing = -controller.GetLeftThumbPosition().CalcHeadingDegrees();
        ship->SetOrientationDegrees(newFacing);
    }
    return {};
}

std::unique_ptr<GameState> MainState::HandleMouseInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(!g_theGame->IsMouseActive()) {
        return {};
    }
    if(!ship) {
        return {};
    }
    if(g_theInputSystem->WasMouseMoved()) {
        const auto& camera = m_cameraController.GetCamera();
        auto mouseWorldCoords = g_theRenderer->ConvertScreenToWorldCoords(camera, g_theInputSystem->GetCursorWindowPosition());
        const auto newFacing = (mouseWorldCoords - ship->GetPosition()).CalcHeadingDegrees();
        ship->SetOrientationDegrees(newFacing);
    }

    if(g_theInputSystem->IsKeyDown(KeyCode::LButton)) {
        ship->OnFire();
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::MButton)) {
        ship->DropMine();
    }
    if(g_theInputSystem->IsKeyDown(KeyCode::RButton)) {
        ship->Thrust(m_thrust_force);
    } else {
        ship->Thrust(0.0f);
    }
    if(g_theInputSystem->WasMouseWheelJustScrolledDown()) {
        m_cameraController.ZoomOut();
    } else if(g_theInputSystem->WasMouseWheelJustScrolledUp()) {
        m_cameraController.ZoomIn();
    }
    return {};
}

void MainState::HandleDebugInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) {
    HandleDebugKeyboardInput(deltaSeconds);
}

void MainState::HandleDebugKeyboardInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) {
    if(g_theUISystem->WantsInputKeyboardCapture()) {
        return;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F1)) {
        m_debug_render = !m_debug_render;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F2)) {
        g_theGame->easyMode = !g_theGame->easyMode;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F4)) {
        g_theUISystem->ToggleImguiDemoWindow();
    }
}

void MainState::HandlePlayerInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) {
    if(!g_theGame->easyMode) {
        if(auto kb_state = HandleKeyboardInput(deltaSeconds)) {
            g_theGame->ChangeState(std::move(kb_state));
        }
        if(auto mouse_state = HandleMouseInput(deltaSeconds)) {
            g_theGame->ChangeState(std::move(mouse_state));
        }
        if(auto ctrl_state = HandleControllerInput(deltaSeconds)) {
            g_theGame->ChangeState(std::move(ctrl_state));
        }
    } else {
        const auto closest_asteroid_target = [this]()->std::optional<std::pair<Vector2, Vector2>> {
            if(ship) {
                const auto iter = std::min_element(std::cbegin(g_theGame->asteroids), std::cend(g_theGame->asteroids),
                [this](const Asteroid* a, const Asteroid* b) {
                        const auto a_dist = MathUtils::CalcDistanceSquared(ship->GetPosition(), a->GetPosition());
                        const auto b_dist = MathUtils::CalcDistanceSquared(ship->GetPosition(), b->GetPosition());
                        return a_dist < b_dist;
                });
                if(iter == std::cend(g_theGame->asteroids)) {
                    return {};
                }
                return std::make_optional(std::make_pair((*iter)->GetPosition(), (*iter)->GetVelocity()));
            }
            return {};
        }();
        const auto closest_ufo_target = [this]()->std::optional<std::pair<Vector2, Vector2>> {
            if(ship) {
                const auto iter = std::min_element(std::cbegin(g_theGame->ufos), std::cend(g_theGame->ufos),
                    [this](const Ufo* a, const Ufo* b) {
                        const auto a_dist = MathUtils::CalcDistanceSquared(ship->GetPosition(), a->GetPosition());
                        const auto b_dist = MathUtils::CalcDistanceSquared(ship->GetPosition(), b->GetPosition());
                        return a_dist < b_dist;
                    });
                if(iter == std::cend(g_theGame->ufos)) {
                    return {};
                }
                return std::make_optional(std::make_pair((*iter)->GetPosition(), (*iter)->GetVelocity()));
            }
            return {};
        }();
        auto calculate_closest_target = [closest_asteroid_target, closest_ufo_target]()->std::optional<std::pair<Vector2, Vector2>> {
            if(closest_asteroid_target && closest_ufo_target) {
                if(closest_asteroid_target->first.CalcLengthSquared() < closest_ufo_target->first.CalcLengthSquared()) {
                    return closest_asteroid_target;
                } else {
                    return closest_ufo_target;
                }
            } else if(closest_ufo_target) {
                return closest_ufo_target;
            } else if(closest_asteroid_target) {
                return closest_asteroid_target;
            } else {
                return {};
            }
        };
        if(auto closest_target = calculate_closest_target(); closest_target.has_value()) {
            auto target_speed = closest_target->second.Normalize();
            auto closeness = MathUtils::DotProduct(Vector2::CreateFromPolarCoordinatesDegrees(1.0f, ship->GetOrientationDegrees()), closest_target->second);
            if(closeness > 0.5f) {
                if(closest_target.has_value()) {
                    target_speed = closest_target->second.Normalize();
                    closeness = MathUtils::DotProduct(Vector2::CreateFromPolarCoordinatesDegrees(1.0f, ship->GetOrientationDegrees()), closest_target->second);
                }
            }
            _auto_target_location = closest_target->first + closest_target->second * target_speed * 2.0f;
            ship->SetOrientationDegrees((_auto_target_location - ship->GetPosition()).CalcHeadingDegrees());
            ship->OnFire();
        }
    }
}

void MainState::WrapAroundWorld(Entity* e) noexcept {
    const auto world_left = world_bounds.mins.x;
    const auto world_right = world_bounds.maxs.x;
    const auto world_top = world_bounds.mins.y;
    const auto world_bottom = world_bounds.maxs.y;
    const auto r = e->GetCosmeticRadius();
    auto pos = e->GetPosition();
    const auto entity_right = pos.x + r;
    const auto entity_left = pos.x - r;
    const auto entity_top = pos.y - r;
    const auto entity_bottom = pos.y + r;
    const auto d = 2.0f * r;
    const auto world_width = world_bounds.CalcDimensions().x;
    const auto world_height = world_bounds.CalcDimensions().y;
    if(entity_right < world_left) {
        pos.x += d + world_width;
    }
    if(entity_left > world_right) {
        pos.x -= d + world_width;
    }
    if(entity_bottom < world_top) {
        pos.y += d + world_height;
    }
    if(entity_top > world_bottom) {
        pos.y -= d + world_height;
    }
    e->SetPosition(pos);
}

void MainState::UpdateEntities(TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(g_theGame->asteroids.empty()) {
        StartNewWave(g_theGame->m_current_wave++);
    }
    for(auto& entity : g_theGame->GetEntities()) {
        if(entity) {
            WrapAroundWorld(entity.get());
            entity->Update(deltaSeconds);
        }
    }
    HandleBulletCollision();
    HandleShipCollision();
    HandleMineCollision();
    ClampCameraToWorld();
}

void MainState::StartNewWave(unsigned int wave_number) noexcept {
    for(unsigned int i = 0; i < wave_number * GetWaveMultiplierFromDifficulty(); ++i) {
        g_theGame->MakeLargeAsteroidOffScreen(world_bounds);
    }
}

void MainState::MakeUfo() noexcept {
    MakeUfo(Ufo::Type::Small);
}

void MainState::MakeUfo(Ufo::Type type) noexcept {
    switch(type) {
    case Ufo::Type::Small: g_theGame->MakeSmallUfo(world_bounds); break;
    case Ufo::Type::Big: g_theGame->MakeBigUfo(world_bounds); break;
    case Ufo::Type::Boss: g_theGame->MakeBossUfo(world_bounds); break;
    default: break;
    }

}

void MainState::MakeShip() noexcept {
    if(!ship) {
        if(g_theGame->GetEntities().empty()) {
            g_theGame->GetEntities().emplace_back(std::make_unique<Ship>(world_bounds.CalcCenter()));
        } else {
            auto iter = g_theGame->GetEntities().begin();
            *iter = std::move(std::make_unique<Ship>(world_bounds.CalcCenter()));
        }
        ship = reinterpret_cast<Ship*>(g_theGame->GetEntities().begin()->get());
        ship->OnCreate();
    }
}

void MainState::MakeLargeAsteroidAtMouse() noexcept {
    const auto& camera = m_cameraController.GetCamera();
    const auto mouseWorldCoords = g_theRenderer->ConvertScreenToWorldCoords(camera, g_theInputSystem->GetCursorScreenPosition());
    g_theGame->MakeLargeAsteroidAt(mouseWorldCoords);
}

void MainState::Respawn() noexcept {
    MakeShip();
}

void MainState::HandleBulletCollision() const noexcept {
    HandleBulletAsteroidCollision();
    HandleBulletUfoCollision();
}

void MainState::HandleBulletAsteroidCollision() const noexcept {
    for(auto& bullet : g_theGame->bullets) {
        Disc2 bulletCollisionMesh{bullet->GetPosition(), bullet->GetPhysicalRadius()};
        for(auto& asteroid : g_theGame->asteroids) {
            Disc2 asteroidCollisionMesh{asteroid->GetPosition(), asteroid->GetPhysicalRadius()};
            if(MathUtils::DoDiscsOverlap(bulletCollisionMesh, asteroidCollisionMesh)) {
                asteroid->OnCollision(asteroid, bullet);
            }
        }
    }
}

void MainState::HandleBulletUfoCollision() const noexcept {
    for(auto& ufo : g_theGame->ufos) {
        Disc2 ufoCollisionMesh{ufo->GetPosition(), ufo->GetPhysicalRadius()};
        for(auto& bullet : g_theGame->bullets) {
            if(bullet->faction == ufo->faction) {
                continue;
            }
            Disc2 bulletCollisionMesh{bullet->GetPosition(), bullet->GetPhysicalRadius()};
            if(MathUtils::DoDiscsOverlap(bulletCollisionMesh, ufoCollisionMesh)) {
                ufo->OnCollision(ufo, bullet);
            }
        }
    }
}

void MainState::HandleShipCollision() noexcept {
    HandleShipAsteroidCollision();
    HandleShipBulletCollision();
}

void MainState::HandleShipAsteroidCollision() noexcept {
    if(!ship) {
        return;
    }
    Disc2 shipCollisionMesh{ship->GetPosition(), ship->GetPhysicalRadius()};
    for(auto& asteroid : g_theGame->asteroids) {
        Disc2 asteroidCollisionMesh{asteroid->GetPosition(), asteroid->GetPhysicalRadius()};
        if(MathUtils::DoDiscsOverlap(shipCollisionMesh, asteroidCollisionMesh)) {
            ship->OnCollision(ship, asteroid);
            asteroid->OnCollision(asteroid, ship);
            if(ship && ship->IsDead()) {
                DoCameraShake();
                ship = nullptr;
                break;
            }
        }
    }
}

void MainState::HandleShipBulletCollision() noexcept {
    if(!ship) {
        return;
    }
    const auto shipCollisionMesh = Disc2{ship->GetPosition(), ship->GetPhysicalRadius()};
    for(auto& bullet : g_theGame->bullets) {
        const auto bulletCollisionMesh = Disc2{bullet->GetPosition(), bullet->GetPhysicalRadius()};
        if(MathUtils::DoDiscsOverlap(shipCollisionMesh, bulletCollisionMesh)) {
            ship->OnCollision(ship, bullet);
            if(ship && ship->IsDead()) {
                DoCameraShake();
                ship = nullptr;
                break;
            }
        }
    }
}

void MainState::HandleMineCollision() noexcept {
    HandleMineAsteroidCollision();
    HandleMineUfoCollision();
}

void MainState::HandleMineAsteroidCollision() noexcept {
    for(const auto& mine : g_theGame->mines) {
        const auto mineCollisionMesh = Disc2{mine->GetPosition(), mine->GetPhysicalRadius()};
        for(const auto& asteroid : g_theGame->asteroids) {
            const auto asteroidCollisionMesh = Disc2{asteroid->GetPosition(), asteroid->GetPhysicalRadius()};
            if(MathUtils::DoDiscsOverlap(mineCollisionMesh, asteroidCollisionMesh)) {
                asteroid->OnCollision(asteroid, mine);
            }
        }
    }
}

void MainState::HandleMineUfoCollision() noexcept {
    for(const auto& mine : g_theGame->mines) {
        const auto mineCollisionMesh = Disc2{mine->GetPosition(), mine->GetPhysicalRadius()};
        for(const auto& ufo : g_theGame->ufos) {
            const auto ufoCollisionMesh = Disc2{ufo->GetPosition(), ufo->GetPhysicalRadius()};
            if(MathUtils::DoDiscsOverlap(mineCollisionMesh, ufoCollisionMesh)) {
                ufo->OnCollision(ufo, mine);
            }
        }
    }
}

void MainState::KillAll() noexcept {
    for(auto* asteroid : g_theGame->asteroids) {
        if(asteroid) {
            asteroid->Kill();
        }
    }
    for(auto* bullet : g_theGame->bullets) {
        if(bullet) {
            bullet->Kill();
        }
    }
    for(auto* explosion : g_theGame->explosions) {
        if(explosion) {
            explosion->Kill();
        }
    }
    if(ship) {
        ship->Kill();
    }
}

unsigned int MainState::GetWaveMultiplierFromDifficulty() const noexcept {
    switch(g_theGame->gameOptions.difficulty) {
    case Difficulty::Easy: return 3u;
    case Difficulty::Normal: return 5u;
    case Difficulty::Hard: return 7u;
    default: return 5u;
    }
}

long long MainState::GetLivesFromDifficulty() const noexcept {
    switch(g_theGame->gameOptions.difficulty) {
    case Difficulty::Easy: return 5LL;
    case Difficulty::Normal: return 4LL;
    case Difficulty::Hard: return 3LL;
    default: return 4;
    }
}

void MainState::RenderBackground() const noexcept {
    const float ui_view_height = currentGraphicsOptions.WindowHeight;
    const float ui_view_width = ui_view_height * m_cameraController.GetAspectRatio();
    const auto ui_view_extents = Vector2{ui_view_width, ui_view_height};
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    const auto S = Matrix4::CreateScaleMatrix(ui_view_half_extents * 5.0f);
    const auto R = Matrix4::I;
    const auto T = Matrix4::I;
    const auto M = Matrix4::MakeSRT(S, R, T);
    g_theRenderer->SetModelMatrix(M);
    g_theRenderer->SetMaterial("background");
    g_theRenderer->DrawQuad2D();
}

void MainState::RenderEntities() const noexcept {
    for(const auto& entity : g_theGame->GetEntities()) {
        if(entity) {
            entity->Render(*g_theRenderer);
        }
    }
}

void MainState::DebugRenderEntities() const noexcept {
    g_theRenderer->SetModelMatrix();
    g_theRenderer->SetMaterial("__2D");
    for(const auto& e : g_theGame->GetEntities()) {
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
    g_theRenderer->DrawAABB2(world_bounds, Rgba::Green, Rgba::NoAlpha);
    g_theRenderer->DrawAABB2(g_theGame->CalcOrthoBounds(m_cameraController), Rgba::White, Rgba::NoAlpha);
    g_theRenderer->DrawAABB2(g_theGame->CalcViewBounds(m_cameraController), Rgba::Red, Rgba::NoAlpha);
    g_theRenderer->DrawAABB2(g_theGame->CalcCullBounds(m_cameraController), Rgba::White, Rgba::NoAlpha);
    g_theRenderer->DrawCircle2D(m_cameraController.GetCamera().GetPosition(), 25.0f, Rgba::Pink);
    g_theRenderer->DrawAABB2(CalculateCameraBounds(), Rgba::Periwinkle, Rgba::NoAlpha);
    if(g_theGame->easyMode) {
        g_theRenderer->DrawCircle2D(_auto_target_location, 12.0f, Rgba::Red);
    }
}

AABB2 MainState::CalculateCameraBounds() const noexcept {
    //TODO: Calculate clamped bounds based on view and world dimensions
    const auto view_bounds = g_theGame->CalcViewBounds(m_cameraController);
    const auto zoom_ratio = m_cameraController.GetZoomLevel();
    const auto camera_bounds_dimensions = Vector2{zoom_ratio / m_cameraController.GetAspectRatio(), zoom_ratio / m_cameraController.GetAspectRatio()};
    AABB2 result{};
    result.AddPaddingToSides(camera_bounds_dimensions.x, camera_bounds_dimensions.y);
    result.Translate(world_bounds.CalcCenter());
    return result;
}

void MainState::RenderStatus() const noexcept {
    static Camera2D ui_camera = m_cameraController.GetCamera();
    const float ui_view_height = ui_camera.GetViewHeight();
    const float ui_view_width = ui_view_height * ui_camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ui_view_width, ui_view_height};
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    const auto ui_leftBottom = Vector2{-ui_view_half_extents.x, ui_view_half_extents.y};
    const auto ui_rightTop = Vector2{ui_view_half_extents.x, -ui_view_half_extents.y};
    const auto ui_nearFar = Vector2{0.0f, 1.0f};
    const auto ui_cam_pos = ui_view_half_extents;
    ui_camera.position = ui_cam_pos;
    ui_camera.orientation_degrees = 0.0f;
    ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, MathUtils::M_16_BY_9_RATIO);
    g_theRenderer->SetCamera(ui_camera);

    const auto* font = g_theRenderer->GetFont("System32");
    const auto font_position = ui_cam_pos - ui_view_half_extents + Vector2{5.0f, font->GetLineHeight() * 0.0f};

    g_theRenderer->SetModelMatrix();
    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(font_position));
    g_theRenderer->DrawMultilineText(g_theRenderer->GetFont("System32"), "Score: " + std::to_string(g_theGame->player.GetScore()) + "\n      x" + std::to_string(g_theGame->player.GetLives()));

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

void MainState::DoCameraShake() noexcept {
    g_theGame->DoCameraShake(m_cameraController);
}

void MainState::ClampCameraToWorld() noexcept {
    const auto camera_limits = CalculateCameraBounds();
    const auto current_ship_position = Vector2::ZERO;//ship ? ship->GetPosition() : Vector2::ZERO;
    const auto clamped_position = MathUtils::CalcClosestPoint(current_ship_position, camera_limits);
    m_cameraController.TranslateTo(clamped_position, g_theRenderer->GetGameFrameTime() * 5.0f);
}


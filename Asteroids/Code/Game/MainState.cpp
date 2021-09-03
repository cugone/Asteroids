#include "Game/MainState.hpp"

#include "Engine/Core/EngineCommon.hpp"
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

#include <utility>

void MainState::OnEnter() noexcept {

    world_bounds = AABB2::Zero_to_One;
    auto dims = Vector2{g_theRenderer->GetOutput()->GetDimensions()};
    //TODO: Fix world dims
    world_bounds.ScalePadding(dims.x, dims.y);
    world_bounds.Translate(-world_bounds.CalcCenter());

    m_cameraController = OrthographicCameraController{};
    m_cameraController.SetPosition(world_bounds.CalcCenter());
    m_cameraController.SetZoomLevelRange(Vector2{225.0f, 450.0f});
    m_cameraController.SetZoomLevel(450.0f);

    PlayerDesc playerDesc{};
    playerDesc.lives = GetLivesFromDifficulty();
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        game->player = Player{playerDesc};

        game->particleSystem->RegisterEffectsFromFolder(FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::GameData) / "ParticleEffects");
    }
    MakeShip();
}

void MainState::OnExit() noexcept {
    m_debug_render = false;
    if(g_theUISystem->IsImguiDemoWindowVisible()) {
        g_theUISystem->ToggleImguiDemoWindow();
    }
    if(g_theUISystem->IsImguiMetricsWindowVisible()) {
        g_theUISystem->ToggleImguiMetricsWindow();
    }
    //auto* group = g_theAudioSystem->GetChannelGroup(g_audiogroup_sound);
    //group->Stop();
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        game->asteroids.clear();
        game->asteroids.shrink_to_fit();
        game->bullets.clear();
        game->bullets.shrink_to_fit();
        game->explosions.clear();
        game->explosions.shrink_to_fit();
        game->ufos.clear();
        game->ufos.shrink_to_fit();
        game->GetEntities().clear();
        game->GetEntities().shrink_to_fit();
        game->m_current_wave = 1u;
        ship = nullptr;
    }
}

void MainState::BeginFrame() noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        game->SetControlType();
        for(auto& entity : game->GetEntities()) {
            if(entity) {
                entity->BeginFrame();
            }
        }
        if(!ship) {
            if(game->respawnTimer.CheckAndReset()) {
                Respawn();
            }
        }
    }
}

void MainState::Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        if(game->IsPaused()) {
            deltaSeconds = deltaSeconds.zero();
        }
        g_theRenderer->UpdateGameTime(deltaSeconds);
        HandleDebugInput(deltaSeconds);
        HandlePlayerInput(deltaSeconds);
        UpdateEntities(deltaSeconds);

        if(game->IsGameOver()) {
            if(DoFadeOut(deltaSeconds)) {
                game->ChangeState(std::move(std::make_unique<GameOverState>()));
            }
        }

        m_cameraController.Update(deltaSeconds);
    }
}

void MainState::Render() const noexcept {
    g_theRenderer->SetRenderTargetsToBackBuffer();
    g_theRenderer->ClearDepthStencilBuffer();

    g_theRenderer->ClearColor(Rgba::Black);

    g_theRenderer->SetViewportAsPercent();

    RenderBackground();
    RenderEntities();
    DebugRenderEntities();
    RenderStatus();
    RenderFadeOutOverlay();
    RenderPausedOverlay();
}

void MainState::RenderFadeOutOverlay() const noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        if(!game->IsGameOver()) {
            return;
        }
        const auto ui_view_height = static_cast<float>(game->gameOptions.GetWindowHeight());
        const auto ui_view_width = ui_view_height * m_cameraController.GetAspectRatio();
        const auto ui_view_extents = Vector2{ui_view_width, ui_view_height};
        const auto ui_view_half_extents = ui_view_extents * 0.5f;
        const auto S = Matrix4::CreateScaleMatrix(ui_view_half_extents * 5.0f);
        const auto R = Matrix4::I;
        const auto T = Matrix4::I;
        const auto M = Matrix4::MakeSRT(S, R, T);
        g_theRenderer->SetModelMatrix(M);
        g_theRenderer->SetMaterial("__2D");
        g_theRenderer->DrawQuad2D(Rgba{0.0f, 0.0f, 0.0f, m_fadeOut_alpha});
    }
}

void MainState::EndFrame() noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        for(auto& entity : game->GetEntities()) {
            if(entity) {
                entity->EndFrame();
            }
        }
        for(auto& entity : game->GetEntities()) {
            if(entity && entity->IsDead()) {
                entity->OnDestroy();
                entity.reset();
            }
        }
        game->PostFrameCleanup();
    }
}

std::unique_ptr<GameState> MainState::HandleInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    return{};
}

std::unique_ptr<GameState> MainState::HandleKeyboardInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::Esc)) {
        return std::make_unique<TitleState>();
    }
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        if(g_theInputSystem->WasKeyJustPressed(KeyCode::P)) {
            game->TogglePause();
            return {};
        }
        if(game->IsPaused()) {
            return {};
        }
        if(!game->IsKeyboardActive()) {
            return {};
        }
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
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        if(auto& controller = g_theInputSystem->GetXboxController(0); controller.IsConnected() && controller.WasButtonJustPressed(XboxController::Button::Start)) {
            game->TogglePause();
            return {};
        }
        if(!game->IsControllerActive()) {
            return {};
        }
        if(game->IsPaused()) {
            return {};
        }
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
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        if(!game->IsMouseActive()) {
            return {};
        }
        if(game->IsPaused()) {
            return {};
        }
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
#ifdef RENDER_DEBUG
    if(g_theUISystem->WantsInputKeyboardCapture()) {
        return;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F1)) {
        m_debug_render = !m_debug_render;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F3)) {
        g_theUISystem->ToggleImguiMetricsWindow();
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F4)) {
        g_theUISystem->ToggleImguiDemoWindow();
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::J)) {
        MakeUfo(Ufo::Type::Small);
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::K)) {
        MakeUfo(Ufo::Type::Big);
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::L)) {
        MakeUfo(Ufo::Type::Boss);
    }
#endif
}

void MainState::HandlePlayerInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        if(auto kb_state = HandleKeyboardInput(deltaSeconds)) {
            game->ChangeState(std::move(kb_state));
        }
        if(auto mouse_state = HandleMouseInput(deltaSeconds)) {
            game->ChangeState(std::move(mouse_state));
        }
        if(auto ctrl_state = HandleControllerInput(deltaSeconds)) {
            game->ChangeState(std::move(ctrl_state));
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
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        if(game->asteroids.empty()) {
            StartNewWave(game->m_current_wave++);
        }
        for(auto& entity : game->GetEntities()) {
            if(entity) {
                WrapAroundWorld(entity.get());
                entity->Update(deltaSeconds);
            }
        }
    }
    HandleBulletCollision();
    HandleShipCollision();
    HandleMineCollision();
    ClampCameraToWorld();
}

void MainState::StartNewWave(unsigned int wave_number) noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        for(unsigned int i = 0; i < wave_number * GetWaveMultiplierFromDifficulty(); ++i) {
            game->MakeLargeAsteroidOffScreen(world_bounds);
        }
    }
}

void MainState::MakeUfo() noexcept {
    MakeUfo(Ufo::Type::Small);
}

void MainState::MakeUfo(Ufo::Type type) noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        switch(type) {
        case Ufo::Type::Small: game->MakeSmallUfo(world_bounds); break;
        case Ufo::Type::Big: game->MakeBigUfo(world_bounds); break;
        case Ufo::Type::Boss: game->MakeBossUfo(world_bounds); break;
        default: break;
        }
    }
}

void MainState::MakeShip() noexcept {
    if(!ship) {
        if(auto* game = GetGameAs<Game>(); game != nullptr) {
            if(game->GetEntities().empty()) {
                game->GetEntities().emplace_back(std::make_unique<Ship>(world_bounds.CalcCenter()));
            } else {
                auto iter = game->GetEntities().begin();
                *iter = std::move(std::make_unique<Ship>(world_bounds.CalcCenter()));
            }
            ship = reinterpret_cast<Ship*>(game->GetEntities().begin()->get());
            ship->OnCreate();
        }
    }
}

void MainState::MakeLargeAsteroidAtMouse() noexcept {
    const auto& camera = m_cameraController.GetCamera();
    const auto mouseWorldCoords = g_theRenderer->ConvertScreenToWorldCoords(camera, g_theInputSystem->GetCursorScreenPosition());
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        game->MakeLargeAsteroidAt(mouseWorldCoords);
    }
}

void MainState::Respawn() noexcept {
    MakeShip();
}

void MainState::HandleBulletCollision() const noexcept {
    HandleBulletAsteroidCollision();
    HandleBulletUfoCollision();
}

void MainState::HandleBulletAsteroidCollision() const noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        for(auto& bullet : game->bullets) {
            Disc2 bulletCollisionMesh{bullet->GetPosition(), bullet->GetPhysicalRadius()};
            for(auto& asteroid : game->asteroids) {
                Disc2 asteroidCollisionMesh{asteroid->GetPosition(), asteroid->GetPhysicalRadius()};
                if(MathUtils::DoDiscsOverlap(bulletCollisionMesh, asteroidCollisionMesh)) {
                    asteroid->OnCollision(asteroid, bullet);
                }
            }
        }
    }
}

void MainState::HandleBulletUfoCollision() const noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        for(auto& ufo : game->ufos) {
            Disc2 ufoCollisionMesh{ufo->GetPosition(), ufo->GetPhysicalRadius()};
            for(auto& bullet : game->bullets) {
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
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        for(auto& asteroid : game->asteroids) {
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
}

void MainState::HandleShipBulletCollision() noexcept {
    if(!ship) {
        return;
    }
    const auto shipCollisionMesh = Disc2{ship->GetPosition(), ship->GetPhysicalRadius()};
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        for(auto& bullet : game->bullets) {
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
}

void MainState::HandleMineCollision() noexcept {
    HandleMineAsteroidCollision();
    HandleMineUfoCollision();
}

void MainState::HandleMineAsteroidCollision() noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        for(const auto& mine : game->mines) {
            const auto mineCollisionMesh = Disc2{mine->GetPosition(), mine->GetPhysicalRadius()};
            for(const auto& asteroid : game->asteroids) {
                const auto asteroidCollisionMesh = Disc2{asteroid->GetPosition(), asteroid->GetPhysicalRadius()};
                if(MathUtils::DoDiscsOverlap(mineCollisionMesh, asteroidCollisionMesh)) {
                    asteroid->OnCollision(asteroid, mine);
                }
            }
        }
    }
}

void MainState::HandleMineUfoCollision() noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        for(const auto& mine : game->mines) {
            const auto mineCollisionMesh = Disc2{mine->GetPosition(), mine->GetPhysicalRadius()};
            for(const auto& ufo : game->ufos) {
                const auto ufoCollisionMesh = Disc2{ufo->GetPosition(), ufo->GetPhysicalRadius()};
                if(MathUtils::DoDiscsOverlap(mineCollisionMesh, ufoCollisionMesh)) {
                    ufo->OnCollision(ufo, mine);
                }
            }
        }
    }
}

void MainState::KillAll() noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        for(auto* asteroid : game->asteroids) {
            if(asteroid) {
                asteroid->Kill();
            }
        }
        for(auto* bullet : game->bullets) {
            if(bullet) {
                bullet->Kill();
            }
        }
        for(auto* explosion : game->explosions) {
            if(explosion) {
                explosion->Kill();
            }
        }
    }
    if(ship) {
        ship->Kill();
    }
}

unsigned int MainState::GetWaveMultiplierFromDifficulty() const noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        switch(game->gameOptions.GetDifficulty()) {
        case Difficulty::Easy: return 3u;
        case Difficulty::Normal: return 5u;
        case Difficulty::Hard: return 7u;
        default: return 5u;
        }
    }
    return 5u;
}

long long MainState::GetLivesFromDifficulty() const noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        switch(game->gameOptions.GetDifficulty()) {
        case Difficulty::Easy: return 5LL;
        case Difficulty::Normal: return 4LL;
        case Difficulty::Hard: return 3LL;
        default: return 4LL;
        }
    }
    return 4LL;
}

void MainState::RenderBackground() const noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        const auto ui_view_height = static_cast<float>(game->gameOptions.GetWindowHeight());
        const auto ui_view_width = ui_view_height * m_cameraController.GetAspectRatio();
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
}

void MainState::RenderEntities() const noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        for(const auto& entity : game->GetEntities()) {
            if(entity) {
                entity->Render();
            }
        }
    }
}

void MainState::DebugRenderEntities() const noexcept {
    if(!m_debug_render) {
        return;
    }
    g_theRenderer->SetModelMatrix();
    g_theRenderer->SetMaterial("__2D");
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        for(const auto& e : game->GetEntities()) {
            if(!e) {
                continue;
            }
            const auto* entity = e.get();
            const auto center = entity->GetPosition();
            const auto orientation = entity->GetOrientationDegrees();
            const auto cosmetic_radius = entity->GetCosmeticRadius();
            const auto physical_radius = entity->GetPhysicalRadius();
            const auto facing_end = [=]()->Vector2 { auto end = Vector2::X_Axis; end.SetLengthAndHeadingDegrees(orientation, cosmetic_radius); return center + end; }();
            const auto velocity_end = [=]()->Vector2 { auto end = entity->GetVelocity().GetNormalize(); end.SetLengthAndHeadingDegrees(end.CalcHeadingDegrees(), cosmetic_radius); return center + end; }();
            const auto acceleration_end = [=]()->Vector2 { auto end = entity->GetAcceleration().GetNormalize(); end.SetLengthAndHeadingDegrees(end.CalcHeadingDegrees(), cosmetic_radius); return center + end; }();
            g_theRenderer->DrawCircle2D(center, cosmetic_radius, Rgba::Green);
            g_theRenderer->DrawCircle2D(center, physical_radius, Rgba::Red);
            g_theRenderer->DrawLine2D(center, facing_end, Rgba::Red);
            g_theRenderer->DrawLine2D(center, velocity_end, Rgba::Green);
            g_theRenderer->DrawLine2D(center, acceleration_end, Rgba::Orange);
        }
        g_theRenderer->DrawAABB2(world_bounds, Rgba::Green, Rgba::NoAlpha);
        g_theRenderer->DrawAABB2(game->CalcOrthoBounds(m_cameraController), Rgba::White, Rgba::NoAlpha);
        g_theRenderer->DrawAABB2(game->CalcViewBounds(m_cameraController), Rgba::Red, Rgba::NoAlpha);
        g_theRenderer->DrawAABB2(game->CalcCullBounds(m_cameraController), Rgba::White, Rgba::NoAlpha);
        g_theRenderer->DrawCircle2D(m_cameraController.GetCamera().GetPosition(), 25.0f, Rgba::Pink);
        g_theRenderer->DrawAABB2(CalculateCameraBounds(), Rgba::Periwinkle, Rgba::NoAlpha);
    }
}

AABB2 MainState::CalculateCameraBounds() const noexcept {
    //TODO: Calculate clamped bounds based on view and world dimensions
    const auto view_bounds = [this]() {
        if(auto* game = GetGameAs<Game>(); game != nullptr) {
            return game->CalcViewBounds(m_cameraController);
        }
        return AABB2{};
    }(); //IIIL
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
    const auto [playerScore, playerLives] = []() {
        if(auto* game = GetGameAs<Game>(); game != nullptr) {
            return std::pair<const long long, const long long>(game->player.GetScore(), game->player.GetLives());
        }
        return std::pair<const long long, const long long>(0LL, 0LL);
    }(); //IIIL
    g_theRenderer->DrawMultilineText(g_theRenderer->GetFont("System32"), "Score: " + std::to_string(playerScore) + "\n      x" + std::to_string(playerLives));

    const auto uvs = AABB2::Zero_to_One;
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
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        game->DoCameraShake(m_cameraController);
    }
}

bool MainState::DoFadeOut(TimeUtils::FPSeconds deltaSeconds) noexcept {
    m_fadeOut_alpha += deltaSeconds.count();
    auto alpha = MathUtils::Interpolate(0.0f, 1.0f, m_fadeOut_alpha);
    alpha = std::clamp(alpha, 0.0f, 1.0f);
    return alpha == 1.0f;
}

void MainState::RenderPausedOverlay() const noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        if(!game->IsPaused()) {
            return;
        }
        const auto ui_view_height = static_cast<float>(game->gameOptions.GetWindowHeight());
        const auto ui_view_width = ui_view_height * m_cameraController.GetAspectRatio();
        const auto ui_view_extents = Vector2{ui_view_width, ui_view_height};
        const auto ui_view_half_extents = ui_view_extents * 0.5f;
        const auto S = Matrix4::CreateScaleMatrix(ui_view_half_extents * 5.0f);
        const auto R = Matrix4::I;
        const auto T = Matrix4::I;
        const auto M = Matrix4::MakeSRT(S, R, T);
        g_theRenderer->SetModelMatrix(M);
        g_theRenderer->SetMaterial("__2D");
        g_theRenderer->DrawQuad2D(Rgba{0.0f, 0.0f, 0.0f, 0.5f});
    }
}

void MainState::ClampCameraToWorld() noexcept {
    const auto camera_limits = CalculateCameraBounds();
    const auto current_ship_position = Vector2::Zero;//ship ? ship->GetPosition() : Vector2::ZERO;
    const auto clamped_position = MathUtils::CalcClosestPoint(current_ship_position, camera_limits);
    m_cameraController.TranslateTo(clamped_position, g_theRenderer->GetGameFrameTime() * 5.0f);
}


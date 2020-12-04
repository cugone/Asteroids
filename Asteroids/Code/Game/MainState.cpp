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

#include "Game/TitleState.hpp"
#include "Game/GameOverState.hpp"

void MainState::OnEnter() noexcept {
    m_cameraController = OrthographicCameraController{g_theRenderer, g_theInputSystem};
    m_cameraController.SetMaxZoomLevel(450.0f);
    m_cameraController.SetZoomLevel(450.0f);

    world_bounds = AABB2::ZERO_TO_ONE;
    const auto dims = Vector2{g_theRenderer->GetOutput()->GetDimensions()};
    //TODO: Fix world dims
    world_bounds.ScalePadding(dims.x, dims.y);
    world_bounds.Translate(-world_bounds.CalcCenter());

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
    Utils::DoOnce(
        [&]() {
            m_ui_camera = m_cameraController.GetCamera();
        }
    );
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
    if(ship) {
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
    }
    return{};
}

std::unique_ptr<GameState> MainState::HandleControllerInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(!g_theGame->IsControllerActive()) {
        return {};
    }
    if(ship) {
        if(auto& controller = g_theInputSystem->GetXboxController(0); controller.IsConnected() && controller.IsButtonDown(XboxController::Button::A)) {
            ship->Thrust(m_thrust_force);
        } else {
            ship->Thrust(0.0f);
        }
        if(auto& controller = g_theInputSystem->GetXboxController(0); controller.IsConnected() && controller.GetRightTriggerPosition() > 0.0f) {
            ship->OnFire();
        }
        if(auto& controller = g_theInputSystem->GetXboxController(0); controller.IsConnected() && controller.GetLeftThumbPosition().CalcLengthSquared() > 0.0f) {
            const auto newFacing = controller.GetLeftThumbPosition().CalcHeadingDegrees();
            ship->SetOrientationDegrees(newFacing);
        }
    }
    return {};
}

std::unique_ptr<GameState> MainState::HandleMouseInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(!g_theGame->IsMouseActive()) {
        return {};
    }
    if(ship) {
        if(g_theInputSystem->GetMouseDelta().CalcLengthSquared() > 0.0f) {
            const auto& camera = m_cameraController.GetCamera();
            auto mouseWorldCoords = g_theRenderer->ConvertScreenToWorldCoords(camera, g_theInputSystem->GetCursorScreenPosition());
            const auto newFacing = (mouseWorldCoords - ship->GetPosition()).CalcHeadingDegrees();
            ship->SetOrientationDegrees(newFacing);
        }

        if(g_theInputSystem->IsKeyDown(KeyCode::LButton)) {
            ship->OnFire();
        }
        if(g_theInputSystem->IsKeyDown(KeyCode::RButton)) {
            ship->Thrust(m_thrust_force);
        } else {
            ship->Thrust(0.0f);
        }
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
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F4)) {
        g_theUISystem->ToggleImguiDemoWindow();
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::I)) {
        MakeLargeAsteroidAtMouse();
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::N)) {
        MakeShip();
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::R)) {
        Respawn();
    }
}

void MainState::HandlePlayerInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) {
    if(auto kb_state = HandleKeyboardInput(deltaSeconds)) {
        g_theGame->ChangeState(std::move(kb_state));
    }
    if(auto mouse_state = HandleMouseInput(deltaSeconds)) {
        g_theGame->ChangeState(std::move(mouse_state));
    }
    if(auto ctrl_state = HandleControllerInput(deltaSeconds)) {
        g_theGame->ChangeState(std::move(ctrl_state));
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
    HandleBulletAsteroidCollision();
    HandleShipAsteroidCollision();
    static Vector2 previousPos = world_bounds.CalcCenter();
    Vector2 currentPosition = ship ? ship->GetPosition() : previousPos;
    previousPos = currentPosition;
    ClampCameraToWorld();
}

void MainState::StartNewWave(unsigned int wave_number) noexcept {
    for(unsigned int i = 0; i < wave_number * GetWaveMultiplierFromDifficulty(); ++i) {
        g_theGame->MakeLargeAsteroidOffScreen(world_bounds);
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

void MainState::HandleBulletAsteroidCollision() const noexcept {
    for(auto& bullet : g_theGame->bullets) {
        for(auto& asteroid : g_theGame->asteroids) {
            Disc2 bulletCollisionMesh{bullet->GetPosition(), bullet->GetPhysicalRadius()};
            Disc2 asteroidCollisionMesh{asteroid->GetPosition(), asteroid->GetPhysicalRadius()};
            if(MathUtils::DoDiscsOverlap(bulletCollisionMesh, asteroidCollisionMesh)) {
                bullet->OnCollision(bullet, asteroid);
                asteroid->OnCollision(asteroid, bullet);
            }
        }
    }
}

void MainState::HandleShipAsteroidCollision() noexcept {
    if(!ship) {
        return;
    }
    for(auto& asteroid : g_theGame->asteroids) {
        if(!ship) break;
        Disc2 shipCollisionMesh{ship->GetPosition(), ship->GetPhysicalRadius()};
        Disc2 asteroidCollisionMesh{asteroid->GetPosition(), asteroid->GetPhysicalRadius()};
        if(MathUtils::DoDiscsOverlap(shipCollisionMesh, asteroidCollisionMesh)) {
            ship->OnCollision(ship, asteroid);
            asteroid->OnCollision(asteroid, ship);
            g_theGame->DoCameraShake(m_cameraController);
            if(ship && ship->IsDead()) {
                ship = nullptr;
                break;
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
    const auto S = Matrix4::CreateScaleMatrix(ui_view_half_extents * 2.5f);
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
    g_theRenderer->SetModelMatrix(Matrix4::I);
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
}

AABB2 MainState::CalculateCameraBounds() const noexcept {
    //TODO: Calculate clamped bounds based on view and world dimensions
    const auto view_bounds = g_theGame->CalcViewBounds(m_cameraController);
    auto result = world_bounds;
    result.ScalePadding(0.25f, 0.25f);
    result.Translate(world_bounds.CalcCenter());
    return result;
}

void MainState::RenderStatus() const noexcept {
    const float ui_view_height = m_ui_camera.GetViewHeight();
    const float ui_view_width = ui_view_height * m_ui_camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ui_view_width, ui_view_height};
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    const auto ui_leftBottom = Vector2{-ui_view_half_extents.x, ui_view_half_extents.y};
    const auto ui_rightTop = Vector2{ui_view_half_extents.x, -ui_view_half_extents.y};
    const auto ui_nearFar = Vector2{0.0f, 1.0f};
    const auto ui_cam_pos = ui_view_half_extents;
    m_ui_camera.position = ui_cam_pos;
    m_ui_camera.orientation_degrees = 0.0f;
    m_ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, MathUtils::M_16_BY_9_RATIO);
    g_theRenderer->SetCamera(m_ui_camera);

    const auto* font = g_theRenderer->GetFont("System32");
    const auto font_position = ui_cam_pos - ui_view_half_extents + Vector2{5.0f, font->GetLineHeight() * 0.0f};

    g_theRenderer->SetModelMatrix(Matrix4::I);
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

void MainState::ClampCameraToWorld() noexcept {
    const auto camera_limits = CalculateCameraBounds();
    const auto& cameraPos = m_cameraController.GetCamera().GetPosition();
    const auto clamped_position = MathUtils::CalcClosestPoint(cameraPos, camera_limits);
    const auto clamped_displacement = cameraPos - clamped_position;
    const auto current_ship_position = ship ? ship->GetPosition() : Vector2::ZERO;
    m_cameraController.TranslateTo(current_ship_position, g_theRenderer->GetGameTime());
    m_cameraController.SetPosition(clamped_position);
}


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

#include <algorithm>
#include <format>
#include <utility>

void MainState::OnEnter() noexcept {
    m_Scene = std::make_shared<Scene>();
    m_world_bounds = AABB2::Zero_to_One;
    auto dims = Vector2{g_theRenderer->GetOutput()->GetDimensions()};
    //TODO: Fix world dims
    m_world_bounds.ScalePadding(dims.x, dims.y);
    m_world_bounds.Translate(-m_world_bounds.CalcCenter());

    m_cameraController = OrthographicCameraController{};
    m_cameraController.SetPosition(m_world_bounds.CalcCenter());
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
        asteroids.clear();
        asteroids.shrink_to_fit();
        bullets.clear();
        bullets.shrink_to_fit();
        explosions.clear();
        explosions.shrink_to_fit();
        ufos.clear();
        ufos.shrink_to_fit();
        m_entities.clear();
        m_entities.shrink_to_fit();
        m_current_wave = 1u;
        ship = nullptr;
    }
}

void MainState::BeginFrame() noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        game->SetControlType();
        for(auto& entity : m_entities) {
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
        for(auto& entity : m_entities) {
            if(entity) {
                entity->EndFrame();
            }
        }
        for(auto& entity : m_entities) {
            if(entity && entity->IsDead()) {
                entity->OnDestroy();
                entity.reset();
            }
        }
        PostFrameCleanup();
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
        ship->StopThrust();
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
        ship->StopThrust();
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
        ship->StopThrust();
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
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::Semicolon)) {
        KillAll();
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

void MainState::WrapAroundWorld(GameEntity* e) noexcept {
    const auto world_left = m_world_bounds.mins.x;
    const auto world_right = m_world_bounds.maxs.x;
    const auto world_top = m_world_bounds.mins.y;
    const auto world_bottom = m_world_bounds.maxs.y;
    const auto r = e->GetCosmeticRadius();
    auto pos = e->GetPosition();
    const auto entity_right = pos.x + r;
    const auto entity_left = pos.x - r;
    const auto entity_top = pos.y - r;
    const auto entity_bottom = pos.y + r;
    const auto d = 2.0f * r;
    const auto world_width = m_world_bounds.CalcDimensions().x;
    const auto world_height = m_world_bounds.CalcDimensions().y;
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
        if(IsWaveComplete()) {
            StartNewWave(m_current_wave++);
        }
        for(auto& entity : m_entities) {
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
    for(unsigned int i = 0; i < wave_number * GetWaveMultiplierFromDifficulty(); ++i) {
        MakeLargeAsteroidOffScreen(m_world_bounds);
    }
}

void MainState::MakeLargeAsteroidOffScreen(AABB2 world_bounds) noexcept {
    const auto pos = [world_bounds]()->const Vector2 {
        const auto world_dims = world_bounds.CalcDimensions();
        const auto world_width = world_dims.x;
        const auto world_height = world_dims.y;
        const auto left = Vector2{world_bounds.mins.x - Asteroid::largeAsteroidCosmeticSize - 1.0f, MathUtils::GetRandomNegOneToOne<float>() * world_height};
        const auto right = Vector2{world_bounds.maxs.x + Asteroid::largeAsteroidCosmeticSize + 1.0f, MathUtils::GetRandomNegOneToOne<float>() * world_height};
        const auto top = Vector2{MathUtils::GetRandomNegOneToOne<float>() * world_width, world_bounds.mins.y - Asteroid::largeAsteroidCosmeticSize - 1.0f};
        const auto bottom = Vector2{MathUtils::GetRandomNegOneToOne<float>() * world_width, world_bounds.maxs.y + Asteroid::largeAsteroidCosmeticSize + 1.0f };
        const auto i = MathUtils::GetRandomLessThan(4);
        switch(i) {
        case 0:
            return left;
        case 1:
            return right;
        case 2:
            return top;
        case 3:
            return bottom;
        default:
            return left;
        }
    }();
    MakeLargeAsteroidAt(pos);
}

void MainState::MakeLargeAsteroidAt(Vector2 pos) noexcept {
    const auto vx = MathUtils::GetRandomNegOneToOne<float>();
    const auto vy = MathUtils::GetRandomNegOneToOne<float>();
    const auto s = MathUtils::GetRandomInRange<float>(20.0f, 100.0f);
    const auto vel = Vector2{vx, vy} *s;
    const auto rot = MathUtils::GetRandomNegOneToOne<float>() * 180.0f;
    MakeLargeAsteroid(pos, vel, rot);
}

void MainState::MakeLargeAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        game->SetAsteroidSpriteSheet();
    }
    auto newAsteroid = std::make_unique<Asteroid>(m_Scene->get(), Asteroid::Type::Large, pos, vel, rotationSpeed);
    AddNewAsteroidToWorld(std::move(newAsteroid));
}

void MainState::MakeMediumAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        game->SetAsteroidSpriteSheet();
    }

    auto newAsteroid = std::make_unique<Asteroid>(m_Scene, Asteroid::Type::Medium, pos, vel, rotationSpeed);
    AddNewAsteroidToWorld(std::move(newAsteroid));
}

void MainState::MakeSmallAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        game->SetAsteroidSpriteSheet();
    }

    auto newAsteroid = std::make_unique<Asteroid>(m_Scene, Asteroid::Type::Small, pos, vel, rotationSpeed);
    AddNewAsteroidToWorld(std::move(newAsteroid));
}

void MainState::DestroyAsteroid(Asteroid* pAsteroid) noexcept {
    if(pAsteroid && pAsteroid->IsDead()) {
        if(auto iter = std::find(std::begin(asteroids), std::end(asteroids), pAsteroid); iter != std::end(asteroids)) {
            *iter = nullptr;
        }
    }
}

void MainState::AddNewAsteroidToWorld(std::unique_ptr<Asteroid> newAsteroid) {
    auto* last_entity = newAsteroid.get();
    m_pending_entities.emplace_back(std::move(newAsteroid));
    auto* asAsteroid = reinterpret_cast<Asteroid*>(last_entity);
    asteroids.push_back(asAsteroid);
    asAsteroid->OnCreate();
}

Ship* MainState::GetShip() const noexcept {
    return ship;
}

void MainState::MakeExplosion(Vector2 position) noexcept {
    if (auto* game = GetGameAs<Game>(); game) {
        game->SetExplosionSpriteSheet();
    }
    auto newExplosion = std::make_unique<Explosion>(m_Scene, position);
    auto* last_entity = newExplosion.get();
    m_pending_entities.emplace_back(std::move(newExplosion));
    auto* asExplosion = reinterpret_cast<Explosion*>(last_entity);
    explosions.push_back(asExplosion);
    asExplosion->OnCreate();
}

void MainState::DestroyExplosion(Explosion* pExplosion) noexcept {
    if(pExplosion && pExplosion->IsDead()) {
        if(auto iter = std::find(std::begin(explosions), std::end(explosions), pExplosion); iter != std::end(explosions)) {
            *iter = nullptr;
        }
    }
}

void MainState::MakeBullet(const GameEntity* parent, Vector2 pos, Vector2 vel) noexcept {
    auto newBullet = std::make_unique<Bullet>(m_Scene, parent, pos, vel);
    auto* last_entity = newBullet.get();
    m_pending_entities.emplace_back(std::move(newBullet));
    auto* asBullet = reinterpret_cast<Bullet*>(last_entity);
    bullets.push_back(asBullet);
    asBullet->OnCreate();
}

void MainState::DestroyBullet(Bullet* pBullet) noexcept {
    if(pBullet && pBullet->IsDead()) {
        if(auto iter = std::find(std::begin(bullets), std::end(bullets), pBullet); iter != std::end(bullets)) {
            *iter = nullptr;
        }
    }
}

void MainState::MakeMine(const GameEntity* parent, Vector2 position) noexcept {
    if (auto* game = GetGameAs<Game>(); game) {
        game->SetMineSpriteSheet();
    }

    auto newMine = std::make_unique<Mine>(m_Scene, parent, position);
    auto* last_entity = newMine.get();
    m_pending_entities.emplace_back(std::move(newMine));
    auto* asMine = reinterpret_cast<Mine*>(last_entity);
    mines.push_back(asMine);
    asMine->OnCreate();
}

void MainState::DestroyMine(Mine* pMine) noexcept {
    if(pMine && pMine->IsDead()) {
        if(auto iter = std::find(std::begin(mines), std::end(mines), pMine); iter != std::end(mines)) {
            *iter = nullptr;
        }
    }
}

void MainState::MakeSmallUfo(AABB2 world_bounds) noexcept {
    if (auto* game = GetGameAs<Game>(); game) {
        game->SetUfoSpriteSheets();
    }
    MakeUfo(Ufo::Type::Small, world_bounds);
}

void MainState::MakeBigUfo(AABB2 world_bounds) noexcept {
    if (auto* game = GetGameAs<Game>(); game) {
        game->SetUfoSpriteSheets();
    }
    MakeUfo(Ufo::Type::Big, world_bounds);
}

void MainState::MakeBossUfo(AABB2 world_bounds) noexcept {
    if (auto* game = GetGameAs<Game>(); game) {
        game->SetUfoSpriteSheets();
    }
    MakeUfo(Ufo::Type::Boss, world_bounds);
}

void MainState::AddNewUfoToWorld(std::unique_ptr<Ufo> newUfo) noexcept {
    auto* last_entity = newUfo.get();
    m_pending_entities.emplace_back(std::move(newUfo));
    auto* asUfo = reinterpret_cast<Ufo*>(last_entity);
    ufos.push_back(asUfo);
    asUfo->OnCreate();
}

void MainState::MakeUfo() noexcept {
    MakeUfo(Ufo::Type::Small);
}

void MainState::MakeUfo(Ufo::Type type) noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        switch(type) {
        case Ufo::Type::Small: MakeSmallUfo(m_world_bounds); break;
        case Ufo::Type::Big: MakeBigUfo(m_world_bounds); break;
        case Ufo::Type::Boss: MakeBossUfo(m_world_bounds); break;
        default: break;
        }
    }
}

void MainState::MakeUfo(Ufo::Type type, AABB2 world_bounds) noexcept {
    const auto pos = [world_bounds, type]()->const Vector2 {
        const auto world_dims = world_bounds.CalcDimensions();
        const auto world_height = world_dims.y;
        const auto cr = Ufo::GetCosmeticRadiusFromType(type);
        const auto y = [world_height, cr]() {
            const auto r = MathUtils::GetRandomNegOneToOne<float>();
            if(r < 0.0f) {
                return r * world_height + cr;
            }
            return r * world_height - cr;
        }();

        const auto left = Vector2{world_bounds.mins.x, y};
        const auto right = Vector2{world_bounds.maxs.x, y};
        return MathUtils::GetRandomBool() ? left : right;
    }();
    auto newUfo = std::make_unique<Ufo>(m_Scene, type, pos);
    const auto ptr = newUfo.get();
    AddNewUfoToWorld(std::move(newUfo));
    ptr->SetVelocity(Vector2{pos.x < 0.0f ? -ptr->GetSpeed() : ptr->GetSpeed(), 0.0f});
}

void MainState::DestroyUfo(Ufo* pUfo) noexcept {
    if(pUfo && pUfo->IsDead()) {
        if(auto iter = std::find(std::begin(ufos), std::end(ufos), pUfo); iter != std::end(ufos)) {
            *iter = nullptr;
        }
    }
}

void MainState::MakeShip() noexcept {
    if(!ship) {
        if(auto* game = GetGameAs<Game>(); game != nullptr) {
            if(m_entities.empty()) {
                m_entities.emplace_back(std::make_unique<Ship>(m_Scene->get(), m_world_bounds.CalcCenter()));
            } else {
                auto iter = m_entities.begin();
                *iter = std::move(std::make_unique<Ship>(m_Scene, m_world_bounds.CalcCenter()));
            }
            ship = reinterpret_cast<Ship*>(m_entities.begin()->get());
            ship->OnCreate();
        }
    }
}

void MainState::MakeLargeAsteroidAtMouse() noexcept {
    const auto& camera = m_cameraController.GetCamera();
    const auto mouseWorldCoords = g_theRenderer->ConvertScreenToWorldCoords(camera, g_theInputSystem->GetCursorScreenPosition());
    MakeLargeAsteroidAt(mouseWorldCoords);
}

void MainState::Respawn() noexcept {
    MakeShip();
}

void MainState::HandleBulletCollision() const noexcept {
    HandleBulletAsteroidCollision();
    HandleBulletUfoCollision();
}

void MainState::HandleBulletAsteroidCollision() const noexcept {
    for(auto& bullet : bullets) {
        Disc2 bulletCollisionMesh{bullet->GetPosition(), bullet->GetPhysicalRadius()};
        for(auto& asteroid : asteroids) {
            Disc2 asteroidCollisionMesh{asteroid->GetPosition(), asteroid->GetPhysicalRadius()};
            if(MathUtils::DoDiscsOverlap(bulletCollisionMesh, asteroidCollisionMesh)) {
                asteroid->OnCollision(asteroid, bullet);
            }
        }
    }
}

void MainState::HandleBulletUfoCollision() const noexcept {
    for(auto& ufo : ufos) {
        Disc2 ufoCollisionMesh{ufo->GetPosition(), ufo->GetPhysicalRadius()};
        for(auto& bullet : bullets) {
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
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        for(auto& asteroid : asteroids) {
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
        for(auto& bullet : bullets) {
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
        for(const auto& mine : mines) {
            const auto mineCollisionMesh = Disc2{mine->GetPosition(), mine->GetPhysicalRadius()};
            for(const auto& asteroid : asteroids) {
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
        for(const auto& mine : mines) {
            const auto mineCollisionMesh = Disc2{mine->GetPosition(), mine->GetPhysicalRadius()};
            for(const auto& ufo : ufos) {
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
        for(auto* asteroid : asteroids) {
            if(asteroid) {
                asteroid->Kill();
            }
        }
        for(auto* bullet : bullets) {
            if(bullet) {
                bullet->Kill();
            }
        }
        for(auto* explosion : explosions) {
            if(explosion) {
                explosion->Kill();
            }
        }
    }
    if(ship) {
        ship->Kill();
        ship = nullptr;
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
        for(const auto& entity : m_entities) {
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
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        for(const auto& e : m_entities) {
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
            g_theRenderer->SetMaterial("circles");
            g_theRenderer->DrawCircle2D(center, cosmetic_radius, Rgba::Green);
            g_theRenderer->DrawCircle2D(center, physical_radius, Rgba::Red);
            g_theRenderer->SetMaterial("__2D");
            g_theRenderer->DrawLine2D(center, facing_end, Rgba::Red);
            g_theRenderer->DrawLine2D(center, velocity_end, Rgba::Green);
            g_theRenderer->DrawLine2D(center, acceleration_end, Rgba::Orange);
        }
        g_theRenderer->SetMaterial("circles");
        g_theRenderer->DrawCircle2D(m_cameraController.GetCamera().GetPosition(), 25.0f, Rgba::Pink);
        g_theRenderer->SetMaterial("__2D");
        g_theRenderer->DrawAABB2(m_world_bounds, Rgba::Green, Rgba::NoAlpha);
        g_theRenderer->DrawAABB2(game->CalcOrthoBounds(m_cameraController), Rgba::White, Rgba::NoAlpha);
        g_theRenderer->DrawAABB2(game->CalcViewBounds(m_cameraController), Rgba::Red, Rgba::NoAlpha);
        g_theRenderer->DrawAABB2(game->CalcCullBounds(m_cameraController), Rgba::White, Rgba::NoAlpha);
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
    result.Translate(m_world_bounds.CalcCenter());
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
    g_theRenderer->DrawMultilineText(g_theRenderer->GetFont("System32"), std::format("Score: {}\n{:>6}{}", playerScore, 'x', playerLives));

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

void MainState::PostFrameCleanup() noexcept {
    for(auto& e : explosions) {
        DestroyExplosion(e);
    }
    for(auto& e : bullets) {
        DestroyBullet(e);
    }
    for(auto& e : asteroids) {
        DestroyAsteroid(e);
    }
    for(auto& e : ufos) {
        DestroyUfo(e);
    }
    for(auto& e : mines) {
        DestroyMine(e);
    }

    explosions.erase(std::remove_if(std::begin(explosions), std::end(explosions), [&](Explosion* e) { return !e; }), std::end(explosions));
    bullets.erase(std::remove_if(std::begin(bullets), std::end(bullets), [&](Bullet* e) { return !e; }), std::end(bullets));
    asteroids.erase(std::remove_if(std::begin(asteroids), std::end(asteroids), [&](Asteroid* e) { return !e; }), std::end(asteroids));
    ufos.erase(std::remove_if(std::begin(ufos), std::end(ufos), [&](Ufo* e) { return !e; }), std::end(ufos));
    mines.erase(std::remove_if(std::begin(mines), std::end(mines), [&](Mine* e) { return !e; }), std::end(mines));
    m_entities.erase(std::remove_if(std::begin(m_entities) + 1, std::end(m_entities), [&](std::unique_ptr<GameEntity>& e) { return !e; }), std::end(m_entities));

    for(auto&& pending : m_pending_entities) {
        m_entities.emplace_back(std::move(pending));
    }
    m_pending_entities.clear();
}

bool MainState::IsWaveComplete() const noexcept {
    return asteroids.empty();
}

Asteroid* MainState::GetClosestAsteroidToEntity(GameEntity* entity) const noexcept {
    if (asteroids.empty()) {
        return nullptr;
    }
    if (!entity) {
        return nullptr;
    }
    return *std::min_element(std::cbegin(asteroids), std::cend(asteroids), [e = entity](const Asteroid* a, const Asteroid* b) {
        return MathUtils::CalcDistanceSquared(e->GetPosition(), a->GetPosition()) < MathUtils::CalcDistanceSquared(e->GetPosition(), b->GetPosition());
        });
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


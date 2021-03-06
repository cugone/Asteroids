#include "Game/Game.hpp"

#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/KerningFont.hpp"

#include "Engine/Math/Disc2.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Material.hpp"

#include "Engine/UI/UISystem.hpp"

#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"

#include "Game/App.hpp"
#include "Game/Entity.hpp"
#include "Game/Asteroid.hpp"
#include "Game/Bullet.hpp"
#include "Game/Explosion.hpp"
#include "Game/Ship.hpp"
#include "Game/Mine.hpp"

#include "Game/GameState.hpp"
#include "Game/MainState.hpp"
#include "Game/TitleState.hpp"

#include <algorithm>
#include <cmath>

void Game::Initialize() {
    _current_state = std::move(std::make_unique<TitleState>());
    CreateOrLoadOptionsFile();
    g_theRenderer->RegisterMaterialsFromFolder(g_material_folderpath);
    g_theRenderer->SetWindowTitle(g_title_str);
    InitializeAudio();
}

void Game::InitializeAudio() noexcept {
    InitializeSounds();
    InitializeMusic();
    AudioSystem::SoundDesc desc{};
    desc.loopCount = -1;
    desc.frequency = 2.0f;
    //g_theAudioSystem->Play(g_music_bgmpath, desc);
}

void Game::InitializeSounds() noexcept {
    g_theAudioSystem->RegisterWavFilesFromFolder(g_sound_folderpath);
    g_theAudioSystem->AddChannelGroup(g_audiogroup_sound);
    for(const auto& filepath : FileUtils::GetAllPathsInFolders(g_sound_folderpath)) {
        g_theAudioSystem->AddSoundToChannelGroup(g_audiogroup_sound, filepath);
    }
    if(auto* sound_group = g_theAudioSystem->GetChannelGroup(g_audiogroup_sound)) {
        sound_group->SetVolume(gameOptions.soundVolume / 10.0f);
    }
}

void Game::InitializeMusic() noexcept {
    g_theAudioSystem->RegisterWavFilesFromFolder(g_music_folderpath);
    g_theAudioSystem->AddChannelGroup(g_audiogroup_music);
    for(const auto& filepath : FileUtils::GetAllPathsInFolders(g_music_folderpath)) {
        g_theAudioSystem->AddSoundToChannelGroup(g_audiogroup_music, filepath);
    }
    if(auto* music_group = g_theAudioSystem->GetChannelGroup(g_audiogroup_music)) {
        music_group->SetVolume(gameOptions.musicVolume / 10.0f);
    }
}

void Game::BeginFrame() {
    if(_next_state) {
        _current_state->OnExit();
        _current_state = std::move(_next_state);
        _current_state->OnEnter();
        _next_state.reset(nullptr);
    }
    _current_state->BeginFrame();
}

void Game::ChangeState(std::unique_ptr<GameState> newState) noexcept {
    _next_state = std::move(newState);
}

void Game::DecrementLives() noexcept {
    player.DecrementLives();
}

bool Game::IsGameOver() const noexcept {
    return player.desc.lives == 0;
}

void Game::SetAsteroidSpriteSheet() noexcept {
    if(!asteroid_sheet) {
        asteroid_sheet = g_theRenderer->CreateSpriteSheet("Data/Images/asteroid.png", 6, 5);
    }
}

void Game::SetMineSpriteSheet() noexcept {
    if(!mine_sheet) {
        mine_sheet = g_theRenderer->CreateSpriteSheet("Data/Images/mine.png", 3, 4);
    }
}

void Game::SetExplosionSpriteSheet() noexcept {
    if(!explosion_sheet) {
        explosion_sheet = g_theRenderer->CreateSpriteSheet("Data/Images/explosion.png", 5, 5);
    }
}

void Game::SetUfoSpriteSheets() noexcept {
    if(!ufo_sheet) {
        ufo_sheet = g_theRenderer->CreateSpriteSheet("Data/Images/ufo.png", 1, 4);
    }
}

AABB2 Game::CalcOrthoBounds(const OrthographicCameraController& controller) const noexcept {
    float half_view_height = controller.GetCamera().GetViewHeight() * 0.5f;
    float half_view_width = half_view_height * controller.GetAspectRatio();
    auto ortho_mins = Vector2{-half_view_width, -half_view_height};
    auto ortho_maxs = Vector2{half_view_width, half_view_height};
    return AABB2{ortho_mins, ortho_maxs};
}

AABB2 Game::CalcViewBounds(const OrthographicCameraController& controller) const noexcept {
    auto view_bounds = CalcOrthoBounds(controller);
    view_bounds.Translate(controller.GetCamera().GetPosition());
    return view_bounds;
}

AABB2 Game::CalcCullBounds(const OrthographicCameraController& controller) const noexcept {
    AABB2 cullBounds = CalcViewBounds(controller);
    cullBounds.AddPaddingToSides(-1.0f, -1.0f);
    return cullBounds;
}

AABB2 Game::CalcCullBoundsFromOrthoBounds(const OrthographicCameraController& controller) const noexcept {
    AABB2 cullBounds = CalcOrthoBounds(controller);
    cullBounds.AddPaddingToSides(-1.0f, -1.0f);
    return cullBounds;
}

void Game::AddNewUfoToWorld(std::unique_ptr<Ufo> newUfo) noexcept {
    auto* last_entity = newUfo.get();
    m_pending_entities.emplace_back(std::move(newUfo));
    auto* asUfo = reinterpret_cast<Ufo*>(last_entity);
    ufos.push_back(asUfo);
    asUfo->OnCreate();
}

void Game::MakeUfo(Ufo::Type type, AABB2 world_bounds) noexcept {
    const auto pos = [world_bounds, type]()->const Vector2 {
        const auto world_dims = world_bounds.CalcDimensions();
        const auto world_width = world_dims.x;
        const auto world_height = world_dims.y;
        const auto cr = Ufo::GetCosmeticRadiusFromType(type);
        const auto y = [world_height, cr]() {
            const auto r = MathUtils::GetRandomFloatNegOneToOne();
            if(r < 0.0f) {
                return r * world_height + cr;
            }
            return r * world_height - cr;
        }();

        const auto left = Vector2{world_bounds.mins.x, y };
        const auto right = Vector2{world_bounds.maxs.x, y};
        return MathUtils::GetRandomBool() ? left : right;
    }();
    auto newUfo = std::make_unique<Ufo>(type, pos);
    const auto ptr = newUfo.get();
    g_theGame->AddNewUfoToWorld(std::move(newUfo));
    ptr->SetVelocity(Vector2{pos.x < 0.0f ? -ptr->GetSpeed() : ptr->GetSpeed(), 0.0f});
}

void Game::SetControlType() noexcept {
    if(g_theInputSystem->WasAnyKeyPressed()) {
        _keyboard_control_active = true;
        _mouse_control_active = false;
        _controller_control_active = false;
    }
    if(g_theInputSystem->WasMouseJustUsed()) {
        _keyboard_control_active = false;
        _mouse_control_active = true;
        _controller_control_active = false;
    }
    if(g_theInputSystem->WasAnyControllerJustUsed()) {
        _keyboard_control_active = false;
        _mouse_control_active = false;
        _controller_control_active = true;
    }
}

bool Game::IsControllerActive() const noexcept {
    return _controller_control_active;
}

bool Game::IsKeyboardActive() const noexcept {
    return _keyboard_control_active;
}

bool Game::IsMouseActive() const noexcept {
    return _mouse_control_active;
}

void Game::CreateOptionsFile() const noexcept {
    FileUtils::CreateFolders("Data/Config/");
    GUARANTEE_OR_DIE(FileUtils::WriteBufferToFile(g_options_str, g_options_filepath), "Could not create options file.");
}

void Game::LoadOptionsFile() const noexcept {
    GUARANTEE_OR_DIE(g_theConfig->AppendFromFile(g_options_filepath), "Could not load options file.");
}

void Game::CreateOrLoadOptionsFile() noexcept {
    if(std::filesystem::exists(g_options_filepath)) {
        LoadOptionsFile();
    } else {
        CreateOptionsFile();
        LoadOptionsFile();
    }
    g_theConfig->GetValue("cameraShakeStrength", gameOptions.cameraShakeStrength);
    g_theConfig->GetValue("maxShakeOffsetHorizontal", currentGraphicsOptions.MaxShakeOffsetHorizontal);
    g_theConfig->GetValue("maxShakeOffsetVertical", currentGraphicsOptions.MaxShakeOffsetVertical);
    g_theConfig->GetValue("maxShakeAngle", currentGraphicsOptions.MaxShakeAngle);
    g_theConfig->GetValue("sound", gameOptions.soundVolume);
    g_theConfig->GetValue("music", gameOptions.musicVolume);

}

void Game::Update(TimeUtils::FPSeconds deltaSeconds) {
    _current_state->Update(deltaSeconds);
}

void Game::Render() const {
    _current_state->Render();
}

void Game::EndFrame() {
    _current_state->EndFrame();
}

void Game::PostFrameCleanup() noexcept {
    explosions.erase(std::remove_if(std::begin(explosions), std::end(explosions), [&](Explosion* e) { return !e; }), std::end(explosions));
    bullets.erase(std::remove_if(std::begin(bullets), std::end(bullets), [&](Bullet* e) { return !e; }), std::end(bullets));
    asteroids.erase(std::remove_if(std::begin(asteroids), std::end(asteroids), [&](Asteroid* e) { return !e; }), std::end(asteroids));
    ufos.erase(std::remove_if(std::begin(ufos), std::end(ufos), [&](Ufo* e) { return !e; }), std::end(ufos));
    mines.erase(std::remove_if(std::begin(mines), std::end(mines), [&](Mine* e) { return !e; }), std::end(mines));
    m_entities.erase(std::remove_if(std::begin(m_entities) + 1, std::end(m_entities), [&](std::unique_ptr<Entity>& e) { return !e; }), std::end(m_entities));

    for(auto&& pending : m_pending_entities) {
        m_entities.emplace_back(std::move(pending));
    }
    m_pending_entities.clear();
}

std::vector<std::unique_ptr<Entity>>& Game::GetEntities() noexcept {
    return m_entities;
}

Ship* Game::GetShip() const noexcept {
    if(const auto state = dynamic_cast<MainState*>(_current_state.get()); state != nullptr) {
        return state->ship;
    }
    return nullptr;
}

void Game::MakeBullet(const Entity* parent, Vector2 pos, Vector2 vel) noexcept {
    auto newBullet = std::make_unique<Bullet>(parent, pos, vel);
    auto* last_entity = newBullet.get();
    m_pending_entities.emplace_back(std::move(newBullet));
    auto* asBullet = reinterpret_cast<Bullet*>(last_entity);
    bullets.push_back(asBullet);
    asBullet->OnCreate();
}

void Game::MakeMine(const Entity* parent, Vector2 position) noexcept {
    SetMineSpriteSheet();
    auto newMine = std::make_unique<Mine>(parent, position);
    auto* last_entity = newMine.get();
    m_pending_entities.emplace_back(std::move(newMine));
    auto* asMine = reinterpret_cast<Mine*>(last_entity);
    mines.push_back(asMine);
    asMine->OnCreate();
}

void Game::MakeSmallUfo(AABB2 world_bounds) noexcept {
    SetUfoSpriteSheets();
    MakeUfo(Ufo::Type::Small, world_bounds);
}

void Game::MakeBigUfo(AABB2 world_bounds) noexcept {
    SetUfoSpriteSheets();
    MakeUfo(Ufo::Type::Big, world_bounds);
}

void Game::MakeBossUfo(AABB2 world_bounds) noexcept {
    SetUfoSpriteSheets();
    MakeUfo(Ufo::Type::Boss, world_bounds);
}

void Game::AddNewAsteroidToWorld(std::unique_ptr<Asteroid> newAsteroid) {
    auto* last_entity = newAsteroid.get();
    m_pending_entities.emplace_back(std::move(newAsteroid));
    auto* asAsteroid = reinterpret_cast<Asteroid*>(last_entity);
    asteroids.push_back(asAsteroid);
    asAsteroid->OnCreate();
}

void Game::MakeLargeAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept {
    SetAsteroidSpriteSheet();
    auto newAsteroid = std::make_unique<Asteroid>(Asteroid::Type::Large, pos, vel, rotationSpeed);
    AddNewAsteroidToWorld(std::move(newAsteroid));
}

void Game::MakeLargeAsteroidAt(Vector2 pos) noexcept {
    const auto vx = MathUtils::GetRandomFloatNegOneToOne();
    const auto vy = MathUtils::GetRandomFloatNegOneToOne();
    const auto s = MathUtils::GetRandomFloatInRange(20.0f, 100.0f);
    const auto vel = Vector2{vx, vy} * s;
    const auto rot = MathUtils::GetRandomFloatNegOneToOne() * 180.0f;
    MakeLargeAsteroid(pos, vel, rot);
}

void Game::MakeLargeAsteroidOffScreen(AABB2 world_bounds) noexcept {
    const auto pos = [world_bounds]()->const Vector2 {
        const auto world_dims = world_bounds.CalcDimensions();
        const auto world_width = world_dims.x;
        const auto world_height = world_dims.y;
        const auto left = Vector2{world_bounds.mins.x, MathUtils::GetRandomFloatNegOneToOne() * world_height};
        const auto right = Vector2{world_bounds.maxs.x, MathUtils::GetRandomFloatNegOneToOne() * world_height};
        const auto top = Vector2{MathUtils::GetRandomFloatNegOneToOne() * world_width, world_bounds.mins.y};
        const auto bottom = Vector2{MathUtils::GetRandomFloatNegOneToOne() * world_width, world_bounds.maxs.y};
        const auto i = MathUtils::GetRandomIntLessThan(4);
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

void Game::MakeMediumAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept {
    SetAsteroidSpriteSheet();
    auto newAsteroid = std::make_unique<Asteroid>(Asteroid::Type::Medium, pos, vel, rotationSpeed);
    AddNewAsteroidToWorld(std::move(newAsteroid));
}

void Game::MakeSmallAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept {
    SetAsteroidSpriteSheet();
    auto newAsteroid = std::make_unique<Asteroid>(Asteroid::Type::Small, pos, vel, rotationSpeed);
    AddNewAsteroidToWorld(std::move(newAsteroid));
}

void Game::DoCameraShake(OrthographicCameraController& controller) const noexcept {
    controller.SetupCameraShake(currentGraphicsOptions.MaxShakeOffsetHorizontal, currentGraphicsOptions.MaxShakeOffsetVertical, currentGraphicsOptions.MaxShakeAngle);
    controller.DoCameraShake([this]() {
        const auto shakeMultiplier = gameOptions.cameraShakeStrength;
        if(const auto* ship = GetShip(); ship) {
            const auto speed = ship->GetVelocity().CalcLength();
            return speed;
        } else {
            return 0.0f;
        }
    });
}

void Game::MakeExplosion(Vector2 position) noexcept {
    SetExplosionSpriteSheet();
    auto newExplosion = std::make_unique<Explosion>(position);
    auto* last_entity = newExplosion.get();
    m_pending_entities.emplace_back(std::move(newExplosion));
    auto* asExplosion = reinterpret_cast<Explosion*>(last_entity);
    explosions.push_back(asExplosion);
    asExplosion->OnCreate();
}


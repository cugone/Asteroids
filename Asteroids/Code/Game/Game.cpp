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

#include <algorithm>
#include <cmath>

TitleMenu& operator++(TitleMenu& mode) noexcept {
    using underlying = std::underlying_type_t<TitleMenu>;
    mode = static_cast<TitleMenu>(static_cast<underlying>(mode) + 1);
    if(mode == TitleMenu::Last_) {
        mode = TitleMenu::First_;
    }
    return mode;
}

TitleMenu operator++(TitleMenu& mode, int) noexcept {
    TitleMenu result = mode;
    ++mode;
    return result;
}

TitleMenu& operator--(TitleMenu& mode) noexcept {
    if(mode == TitleMenu::First_) {
        mode = TitleMenu::Last_;
    }
    using underlying = std::underlying_type_t<TitleMenu>;
    mode = static_cast<TitleMenu>(static_cast<underlying>(mode) - 1);
    return mode;
}

TitleMenu operator--(TitleMenu& mode, int) noexcept {
    TitleMenu result = mode;
    --mode;
    return result;
}


OptionsMenu& operator++(OptionsMenu& mode) noexcept {
    using underlying = std::underlying_type_t<TitleMenu>;
    mode = static_cast<OptionsMenu>(static_cast<underlying>(mode) + 1);
    if(mode == OptionsMenu::Last_) {
        mode = OptionsMenu::First_;
    }
    return mode;
}

OptionsMenu operator++(OptionsMenu& mode, int) noexcept {
    OptionsMenu result = mode;
    ++mode;
    return result;
}

OptionsMenu& operator--(OptionsMenu& mode) noexcept {
    if(mode == OptionsMenu::First_) {
        mode = OptionsMenu::Last_;
    }
    using underlying = std::underlying_type_t<OptionsMenu>;
    mode = static_cast<OptionsMenu>(static_cast<underlying>(mode) - 1);
    return mode;
}

OptionsMenu operator--(OptionsMenu& mode, int) noexcept {
    OptionsMenu result = mode;
    --mode;
    return result;
}

void Game::Initialize() {
    CreateOrLoadOptionsFile();
    g_theRenderer->RegisterMaterialsFromFolder(g_material_folderpath);
    g_theRenderer->SetWindowTitle(g_title_str);
    InitializeAudio();
}

void Game::InitializeAudio() noexcept {
    InitializeSounds();
    InitializeMusic();
}

void Game::InitializeSounds() noexcept {
    g_theAudioSystem->RegisterWavFilesFromFolder(g_sound_folderpath);
    g_theAudioSystem->AddChannelGroup("sound");
    for(const auto filepath : FileUtils::GetAllPathsInFolders(g_sound_folderpath)) {
        g_theAudioSystem->AddSoundToChannelGroup("sound", filepath);
    }
    if(auto* sound_group = g_theAudioSystem->GetChannelGroup("sound")) {
        sound_group->SetVolume(_current_options.soundVolume / static_cast<float>(_max_sound_volume));
    }
}

void Game::InitializeMusic() noexcept {
    g_theAudioSystem->RegisterWavFilesFromFolder(g_music_folderpath);
    g_theAudioSystem->AddChannelGroup("music");
    if(auto* music_group = g_theAudioSystem->GetChannelGroup("music")) {
        music_group->SetVolume(_current_options.musicVolume / static_cast<float>(_max_music_volume));
    }
}

void Game::BeginFrame() {
    if(_current_state != _next_state) {
        OnExitState(_current_state);
        _current_state = _next_state;
        OnEnterState(_current_state);
    }

    switch(_current_state) {
    case GameState::Title: BeginFrame_Title(); return;
    case GameState::Options: BeginFrame_Options(); return;
    case GameState::Main: BeginFrame_Main(); return;
    case GameState::GameOver: BeginFrame_GameOver(); return;
    default: ERROR_AND_DIE("BeginFrame Undefined Game State.");
    }
}

void Game::ChangeState(GameState newState) noexcept {
    _next_state = newState;
}

void Game::OnEnterState(GameState state) noexcept {
    switch(state) {
    case GameState::Title: OnEnter_Title(); return;
    case GameState::Options: OnEnter_Options(); return;
    case GameState::Main: OnEnter_Main(); return;
    case GameState::GameOver: OnEnter_GameOver(); return;
    default: ERROR_AND_DIE("OnEnterState: Undefined state");
    }
}

void Game::OnExitState(GameState state) noexcept {
    switch(state) {
    case GameState::Title: OnExit_Title(); return;
    case GameState::Options: OnExit_Options(); return;
    case GameState::Main: OnExit_Main(); return;
    case GameState::GameOver: OnExit_GameOver(); return;
    }
}

void Game::BeginFrame_Title() noexcept {
    SetControlType();
}

void Game::BeginFrame_Options() noexcept {
    SetControlType();
}

void Game::BeginFrame_Main() noexcept {
    SetControlType();
    for(auto& entity : _entities) {
        if(entity) {
            entity->BeginFrame();
        }
    }
}

void Game::BeginFrame_GameOver() noexcept {
    SetControlType();
}

void Game::SetControlType() noexcept {
    if(g_theInputSystem->WasAnyKeyPressed()) {
        _keyboard_control_active = true;
        _mouse_control_active = false;
        _controller_control_active = false;
    }
    if(g_theInputSystem->WasAnyMouseButtonPressed()) {
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

std::string Game::DifficultyToString(Difficulty difficulty) const noexcept {
    switch(difficulty) {
    case Difficulty::Easy: return "Easy";
    case Difficulty::Normal: return "Normal";
    case Difficulty::Hard: return "Hard";
    default: return "";
    }
}

std::string Game::ControlPreferenceToString(ControlPreference preference) const noexcept {
    switch(preference) {
    case ControlPreference::Keyboard: return "Keyboard";
    case ControlPreference::Mouse: return "Mouse";
    case ControlPreference::XboxController: return "Xbox Controller";
    default: return "";
    }
}

void Game::CycleSelectedOptionDown(OptionsMenu selectedItem) noexcept {
    switch(selectedItem) {
    case OptionsMenu::DifficultySelection:
    {
        switch(_temp_options.difficulty) {
        case Difficulty::Easy:
            _temp_options.difficulty = Difficulty::Hard;
            break;
        case Difficulty::Normal:
            _temp_options.difficulty = Difficulty::Easy;
            break;
        case Difficulty::Hard:
            _temp_options.difficulty = Difficulty::Normal;
            break;
        default: /* DO NOTHING */;
        }
        break;
    }
    case OptionsMenu::ControlSelection:
    {
        switch(_temp_options.controlPref) {
        case ControlPreference::Keyboard:
            _temp_options.controlPref = ControlPreference::XboxController;
            break;
        case ControlPreference::Mouse:
            _temp_options.controlPref = ControlPreference::Keyboard;
            break;
        case ControlPreference::XboxController:
            _temp_options.controlPref = ControlPreference::Mouse;
            break;
        default:
            break;
        }
        break;
    }
    case OptionsMenu::SoundVolume:
        --_temp_options.soundVolume;
        _temp_options.soundVolume = std::clamp(_temp_options.soundVolume, _min_sound_volume, _max_sound_volume);
        break;
    case OptionsMenu::MusicVolume:
        --_temp_options.musicVolume;
        _temp_options.musicVolume = std::clamp(_temp_options.musicVolume, _min_music_volume, _max_music_volume);
        break;
    case OptionsMenu::CameraShake:
        _temp_options.cameraShakeStrength -= 0.1f;
        _temp_options.cameraShakeStrength = std::clamp(_temp_options.cameraShakeStrength, _min_camera_shake, _max_camera_shake);
        break;
    default: /* DO NOTHING */;
    }
}

void Game::CycleSelectedOptionUp(OptionsMenu selectedItem) noexcept {
    switch(selectedItem) {
    case OptionsMenu::DifficultySelection:
    {
        switch(_temp_options.difficulty) {
        case Difficulty::Easy:
            _temp_options.difficulty = Difficulty::Normal;
            break;
        case Difficulty::Normal:
            _temp_options.difficulty = Difficulty::Hard;
            break;
        case Difficulty::Hard:
            _temp_options.difficulty = Difficulty::Easy;
            break;
        default: /* DO NOTHING */;
        }
        break;
    }
    case OptionsMenu::ControlSelection:
    {
        switch(_temp_options.controlPref) {
        case ControlPreference::Keyboard:
            _temp_options.controlPref = ControlPreference::Mouse;
            break;
        case ControlPreference::Mouse:
            _temp_options.controlPref = ControlPreference::XboxController;
            break;
        case ControlPreference::XboxController:
            _temp_options.controlPref = ControlPreference::Keyboard;
            break;
        default:
            break;
        }
        break;
    }
    case OptionsMenu::SoundVolume:
        ++_temp_options.soundVolume;
        _temp_options.soundVolume = std::clamp(_temp_options.soundVolume, _min_sound_volume, _max_sound_volume);
        break;
    case OptionsMenu::MusicVolume:
        ++_temp_options.musicVolume;
        _temp_options.musicVolume = std::clamp(_temp_options.musicVolume, _min_music_volume, _max_music_volume);
        break;
    case OptionsMenu::CameraShake:
        _temp_options.cameraShakeStrength += 0.1f;
        _temp_options.cameraShakeStrength = std::clamp(_temp_options.cameraShakeStrength, _min_camera_shake, _max_camera_shake);
        break;
    default: /* DO NOTHING */;
    }
}

unsigned int Game::GetWaveMultiplierFromDifficulty() const noexcept {
    switch(_current_options.difficulty) {
    case Difficulty::Easy: return 3u;
    case Difficulty::Normal: return 5u;
    case Difficulty::Hard: return 7u;
    default: return 5u;
    }
}

AABB2 Game::CalcOrthoBounds() const noexcept {
    float half_view_height = _cameraController.GetCamera().GetViewHeight() * 0.5f;
    float half_view_width = half_view_height * _cameraController.GetAspectRatio();
    auto ortho_mins = Vector2{-half_view_width, -half_view_height};
    auto ortho_maxs = Vector2{half_view_width, half_view_height};
    return AABB2{ortho_mins, ortho_maxs};
}

AABB2 Game::CalcViewBounds(const Vector2& cam_pos) const noexcept {
    auto view_bounds = CalcOrthoBounds();
    view_bounds.Translate(cam_pos);
    return view_bounds;
}

AABB2 Game::CalcCullBounds(const Vector2& cam_pos) const noexcept {
    AABB2 cullBounds = CalcViewBounds(cam_pos);
    cullBounds.AddPaddingToSides(-1.0f, -1.0f);
    return cullBounds;
}

AABB2 Game::CalcCullBoundsFromOrthoBounds() const noexcept {
    AABB2 cullBounds = CalcOrthoBounds();
    cullBounds.AddPaddingToSides(-1.0f, -1.0f);
    return cullBounds;
}

void Game::CreateOptionsFile() const noexcept {
    GUARANTEE_OR_DIE(FileUtils::WriteBufferToFile(g_options_str, g_options_filepath), "Could not create options file.");
}

void Game::LoadOptionsFile() const noexcept {
    g_theConfig->AppendFromFile(g_options_filepath);
}

void Game::CreateOrLoadOptionsFile() noexcept {
    if(std::filesystem::exists(g_options_filepath)) {
        LoadOptionsFile();
    } else {
        CreateOptionsFile();
        LoadOptionsFile();
    }
    g_theConfig->GetValue("cameraShakeStrength", _current_options.cameraShakeStrength);
    g_theConfig->GetValue("maxShakeOffsetHorizontal", currentGraphicsOptions.MaxShakeOffsetHorizontal);
    g_theConfig->GetValue("maxShakeOffsetVertical", currentGraphicsOptions.MaxShakeOffsetVertical);
    g_theConfig->GetValue("maxShakeAngle", currentGraphicsOptions.MaxShakeAngle);
    g_theConfig->GetValue("sound", _current_options.soundVolume);
    g_theConfig->GetValue("music", _current_options.musicVolume);

}

void Game::HandleTitleInput() noexcept {
    HandleTitleKeyboardInput();
    HandleTitleControllerInput();
}

void Game::HandleTitleKeyboardInput() noexcept {
    if(!_keyboard_control_active) {
        return;
    }
    const bool up = g_theInputSystem->WasKeyJustPressed(KeyCode::W) || g_theInputSystem->WasKeyJustPressed(KeyCode::Up);
    const bool down = g_theInputSystem->WasKeyJustPressed(KeyCode::S) || g_theInputSystem->WasKeyJustPressed(KeyCode::Down);
    const bool select = g_theInputSystem->WasKeyJustPressed(KeyCode::Enter) || g_theInputSystem->WasKeyJustPressed(KeyCode::NumPadEnter);
    const bool cancel = g_theInputSystem->WasKeyJustPressed(KeyCode::Esc);
    if(up) {
        --_title_selected_item;

    } else if(down) {
        ++_title_selected_item;
    }
    if(cancel) {
        _title_selected_item = TitleMenu::Exit;
    }
    if(select) {
        switch(_title_selected_item) {
        case TitleMenu::Start:
            ChangeState(GameState::Main);
            break;
        case TitleMenu::Options:
            ChangeState(GameState::Options);
            break;
        case TitleMenu::Exit:
            g_theApp->SetIsQuitting(true);
            break;
        default:
            ERROR_AND_DIE("TITLE MENU ENUM HAS CHANGED.");
        }
    }
}

void Game::HandleTitleControllerInput() noexcept {
    if(const auto controller = g_theInputSystem->GetXboxController(0); _controller_control_active && controller.IsConnected()) {
        const bool up = controller.WasButtonJustPressed(XboxController::Button::Up) || MathUtils::IsEquivalent(1.0f, controller.GetLeftThumbPosition().y);
        const bool down = controller.WasButtonJustPressed(XboxController::Button::Down) || MathUtils::IsEquivalent(-1.0f, controller.GetLeftThumbPosition().y);
        const bool select = g_theInputSystem->WasKeyJustPressed(KeyCode::Enter) || g_theInputSystem->WasKeyJustPressed(KeyCode::NumPadEnter);
        const bool cancel = g_theInputSystem->WasKeyJustPressed(KeyCode::Esc);
        if(up) {
            --_title_selected_item;

        } else if(down) {
            ++_title_selected_item;
        }
        if(cancel) {
            _title_selected_item = TitleMenu::Exit;
        }
        if(select) {
            switch(_title_selected_item) {
            case TitleMenu::Start:
                ChangeState(GameState::Main);
                break;
            case TitleMenu::Options:
                ChangeState(GameState::Options);
                break;
            case TitleMenu::Exit:
                g_theApp->SetIsQuitting(true);
                break;
            default:
                ERROR_AND_DIE("TITLE MENU ENUM HAS CHANGED.");
            }
        }
    }
}

void Game::HandleOptionsInput() noexcept {
    HandleOptionsKeyboardInput();
    HandleOptionsControllerInput();
}

void Game::HandleOptionsKeyboardInput() noexcept {
    if(!_keyboard_control_active) {
        return;
    }
    const bool up = g_theInputSystem->WasKeyJustPressed(KeyCode::W) || g_theInputSystem->WasKeyJustPressed(KeyCode::Up);
    const bool down = g_theInputSystem->WasKeyJustPressed(KeyCode::S) || g_theInputSystem->WasKeyJustPressed(KeyCode::Down);
    const bool left = g_theInputSystem->WasKeyJustPressed(KeyCode::A) || g_theInputSystem->WasKeyJustPressed(KeyCode::Left);
    const bool right = g_theInputSystem->WasKeyJustPressed(KeyCode::D) || g_theInputSystem->WasKeyJustPressed(KeyCode::Right);
    const bool select = g_theInputSystem->WasKeyJustPressed(KeyCode::Enter) || g_theInputSystem->WasKeyJustPressed(KeyCode::NumPadEnter);
    const bool cancel = g_theInputSystem->WasKeyJustPressed(KeyCode::Esc);
    HandleOptionsMenuState(up, down, left, right, cancel, select);
}

void Game::HandleOptionsControllerInput() noexcept {
    if(const auto controller = g_theInputSystem->GetXboxController(0); _controller_control_active && controller.IsConnected()) {
        const bool up = controller.WasButtonJustPressed(XboxController::Button::Up) || MathUtils::IsEquivalent(1.0f, controller.GetLeftThumbPosition().y);
        const bool down = controller.WasButtonJustPressed(XboxController::Button::Down) || MathUtils::IsEquivalent(-1.0f, controller.GetLeftThumbPosition().y);
        const bool left = controller.WasButtonJustPressed(XboxController::Button::Left) || MathUtils::IsEquivalent(-1.0f, controller.GetLeftThumbPosition().x);
        const bool right = controller.WasButtonJustPressed(XboxController::Button::Right) || MathUtils::IsEquivalent(1.0f, controller.GetLeftThumbPosition().x);
        const bool select = controller.WasButtonJustPressed(XboxController::Button::A);
        const bool cancel = controller.WasButtonJustPressed(XboxController::Button::B);
        HandleOptionsMenuState(up, down, left, right, cancel, select);
    }
}

void Game::HandleOptionsMenuState(const bool up, const bool down, const bool left, const bool right, const bool cancel, const bool select) noexcept {
    if(up) {
        --_options_selected_item;
    } else if(down) {
        ++_options_selected_item;
    }
    if(left) {
        CycleSelectedOptionDown(_options_selected_item);
    } else if(right) {
        CycleSelectedOptionUp(_options_selected_item);
    }
    if(cancel) {
        ChangeState(GameState::Title);
        return;
    }
    if(select) {
        switch(_options_selected_item) {
        case OptionsMenu::Cancel:
        {
            ChangeState(GameState::Title);
            break;
        }
        case OptionsMenu::Accept:
        {
            _current_options = _temp_options;
            g_theConfig->SetValue("difficulty", DifficultyToString(_current_options.difficulty));
            g_theConfig->SetValue("controlpref", ControlPreferenceToString(_current_options.controlPref));
            g_theConfig->SetValue("sound", static_cast<int>(_current_options.soundVolume));
            g_theConfig->SetValue("music", static_cast<int>(_current_options.musicVolume));
            g_theConfig->SetValue("camerashakestrength", _current_options.cameraShakeStrength);
            std::ofstream ofs(g_options_filepath);
            ofs << *g_theConfig;
            ofs.close();
            ChangeState(GameState::Title);
            break;
        }
        default:
        {
            ERROR_AND_DIE("OPTIONS MENU ENUM HAS CHANGED.");
        }
        }
    }
}

long long Game::GetLivesFromDifficulty() const noexcept {
    switch(_current_options.difficulty) {
    case Difficulty::Easy: return 5LL;
    case Difficulty::Normal: return 4LL;
    case Difficulty::Hard: return 3LL;
    default: return 4;
    }
}

void Game::Update(TimeUtils::FPSeconds deltaSeconds) {
    switch(_current_state) {
    case GameState::Title: Update_Title(deltaSeconds); return;
    case GameState::Options: Update_Options(deltaSeconds); return;
    case GameState::Main: Update_Main(deltaSeconds); return;
    case GameState::GameOver: Update_GameOver(deltaSeconds); return;
    default: ERROR_AND_DIE("Update Undefined Game State.");
    }
}

void Game::Render() const {
    switch(_current_state) {
    case GameState::Title: Render_Title(); return;
    case GameState::Options: Render_Options(); return;
    case GameState::Main: Render_Main(); return;
    case GameState::GameOver: Render_GameOver(); return;
    default: ERROR_AND_DIE("Render Undefined Game State.");
    }
}

void Game::EndFrame() {
    switch(_current_state) {
    case GameState::Title: EndFrame_Title(); return;
    case GameState::Options: EndFrame_Options(); return;
    case GameState::Main: EndFrame_Main(); return;
    case GameState::GameOver: EndFrame_GameOver(); return;
    default: ERROR_AND_DIE("EndFrame Undefined Game State.");
    }
}

void Game::Update_Title([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    HandleTitleInput();
}

void Game::Update_Options([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    HandleOptionsInput();
}

void Game::Update_Main([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    g_theRenderer->UpdateGameTime(deltaSeconds);
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::Esc)) {
        ChangeState(GameState::Title);
        return;
    }
    if(IsGameOver()) {
        return;
    }
    HandleDebugInput(deltaSeconds);
    HandlePlayerInput(deltaSeconds);
    UpdateEntities(deltaSeconds);

    if(IsGameOver()) {
        ChangeState(GameState::GameOver);
    }

    _cameraController.Update(deltaSeconds);

}

void Game::Update_GameOver([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(g_theInputSystem->WasAnyKeyPressed()) {
        ChangeState(GameState::Title);
    }
}

void Game::Render_Title() const noexcept {
    g_theRenderer->ResetModelViewProjection();
    g_theRenderer->SetRenderTargetsToBackBuffer();
    g_theRenderer->ClearDepthStencilBuffer();

    g_theRenderer->ClearColor(Rgba::Black);

    g_theRenderer->SetViewportAsPercent();

    //2D View / HUD
    const float ui_view_height = currentGraphicsOptions.WindowHeight;
    const float ui_view_width = ui_view_height * _ui_camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ui_view_width, ui_view_height};
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    const auto ui_leftBottom = Vector2{-ui_view_half_extents.x, ui_view_half_extents.y};
    const auto ui_rightTop = Vector2{ui_view_half_extents.x, -ui_view_half_extents.y};
    const auto ui_nearFar = Vector2{0.0f, 1.0f};
    const auto ui_cam_pos = ui_view_half_extents;
    _ui_camera.position = ui_cam_pos;
    _ui_camera.orientation_degrees = 0.0f;
    _ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, MathUtils::M_16_BY_9_RATIO);
    g_theRenderer->SetCamera(_ui_camera);

    const auto* font = g_theRenderer->GetFont("System32");
    const auto line_height = font->GetLineHeight();

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x, ui_view_half_extents.y + line_height * 0.0f}));
    g_theRenderer->DrawTextLine(font, "ASTEROIDS");

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x, ui_view_half_extents.y + line_height * 2.0f}));
    g_theRenderer->DrawTextLine(font, "START", _title_selected_item == TitleMenu::Start ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x, ui_view_half_extents.y + line_height * 4.0f}));
    g_theRenderer->DrawTextLine(font, "OPTIONS", _title_selected_item == TitleMenu::Options ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x, ui_view_half_extents.y + line_height * 6.0f}));
    g_theRenderer->DrawTextLine(font, "EXIT", _title_selected_item == TitleMenu::Exit ? Rgba::Yellow : Rgba::White);

}

void Game::Render_Options() const noexcept {
    g_theRenderer->ResetModelViewProjection();
    g_theRenderer->SetRenderTargetsToBackBuffer();
    g_theRenderer->ClearDepthStencilBuffer();

    g_theRenderer->ClearColor(Rgba::Black);

    g_theRenderer->SetViewportAsPercent();

    //2D View / HUD
    const float ui_view_height = currentGraphicsOptions.WindowHeight;
    const float ui_view_width = ui_view_height * _ui_camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ui_view_width, ui_view_height};
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    auto ui_leftBottom = Vector2{-ui_view_half_extents.x, ui_view_half_extents.y};
    auto ui_rightTop = Vector2{ui_view_half_extents.x, -ui_view_half_extents.y};
    auto ui_nearFar = Vector2{0.0f, 1.0f};
    auto ui_cam_pos = ui_view_half_extents;
    _ui_camera.position = ui_cam_pos;
    _ui_camera.orientation_degrees = 0.0f;
    _ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, MathUtils::M_16_BY_9_RATIO);
    g_theRenderer->SetCamera(_ui_camera);

    const auto* font = g_theRenderer->GetFont("System32");
    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x, ui_view_half_extents.y * 0.25f}));
    g_theRenderer->DrawTextLine(font, "OPTIONS");

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 0.35f}));
    g_theRenderer->DrawTextLine(font, "Difficulty:", _options_selected_item == OptionsMenu::DifficultySelection ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 1.5f, ui_view_half_extents.y * 0.35f}));
    g_theRenderer->DrawTextLine(font, DifficultyToString(_temp_options.difficulty), _options_selected_item == OptionsMenu::DifficultySelection ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 0.45f}));
    g_theRenderer->DrawTextLine(font, "Control Preference:", _options_selected_item == OptionsMenu::ControlSelection ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 1.5f, ui_view_half_extents.y * 0.45f}));
    g_theRenderer->DrawTextLine(font, ControlPreferenceToString(_temp_options.controlPref), _options_selected_item == OptionsMenu::ControlSelection ? Rgba::Yellow : Rgba::White);
    
    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 0.55f}));
    g_theRenderer->DrawTextLine(font, "Camera Shake:", _options_selected_item == OptionsMenu::CameraShake ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 1.5f, ui_view_half_extents.y * 0.55f}));
    {
        std::ostringstream ss;
        ss << std::setprecision(2) << _temp_options.cameraShakeStrength;
        g_theRenderer->DrawTextLine(font, ss.str(), _options_selected_item == OptionsMenu::CameraShake ? Rgba::Yellow : Rgba::White);
    }
    
    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 0.65f}));
    g_theRenderer->DrawTextLine(font, "Sound Volume:", _options_selected_item == OptionsMenu::SoundVolume ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 1.5f, ui_view_half_extents.y * 0.65f}));
    g_theRenderer->DrawTextLine(font, std::to_string(_temp_options.soundVolume), _options_selected_item == OptionsMenu::SoundVolume ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 0.85f}));
    g_theRenderer->DrawTextLine(font, "Back", _options_selected_item == OptionsMenu::Cancel ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 0.95f}));
    g_theRenderer->DrawTextLine(font, "Accept", _options_selected_item == OptionsMenu::Accept ? Rgba::Yellow : Rgba::White);

}

void Game::Render_Main() const noexcept {
    g_theRenderer->SetRenderTargetsToBackBuffer();
    g_theRenderer->ClearDepthStencilBuffer();

    g_theRenderer->ClearColor(Rgba::Black);

    g_theRenderer->SetViewportAsPercent();

    RenderBackground();
    RenderEntities();
    if(_debug_render) {
        DebugRenderEntities();
    }
    RenderStatus();

}

void Game::Render_GameOver() const noexcept {
    g_theRenderer->ResetModelViewProjection();
    g_theRenderer->SetRenderTargetsToBackBuffer();
    g_theRenderer->ClearDepthStencilBuffer();

    g_theRenderer->ClearColor(Rgba::Black);

    g_theRenderer->SetViewportAsPercent();

    //2D View / HUD
    const float ui_view_height = currentGraphicsOptions.WindowHeight;
    const float ui_view_width = ui_view_height * _ui_camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ui_view_width, ui_view_height};
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    auto ui_leftBottom = Vector2{-ui_view_half_extents.x, ui_view_half_extents.y};
    auto ui_rightTop = Vector2{ui_view_half_extents.x, -ui_view_half_extents.y};
    auto ui_nearFar = Vector2{0.0f, 1.0f};
    auto ui_cam_pos = ui_view_half_extents;
    _ui_camera.position = ui_cam_pos;
    _ui_camera.orientation_degrees = 0.0f;
    _ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, MathUtils::M_16_BY_9_RATIO);
    g_theRenderer->SetCamera(_ui_camera);

    const auto* font = g_theRenderer->GetFont("System32");
    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(ui_view_half_extents));
    g_theRenderer->DrawTextLine(font, "GAME OVER");
}

void Game::EndFrame_Title() noexcept {
    /* DO NOTHING */
}

void Game::EndFrame_Options() noexcept {
    /* DO NOTHING */
}

void Game::EndFrame_Main() noexcept {
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
    explosions.erase(std::remove_if(std::begin(explosions), std::end(explosions), [&](Explosion* e) { return !e; }), std::end(explosions));
    bullets.erase(std::remove_if(std::begin(bullets), std::end(bullets), [&](Bullet* e) { return !e; }), std::end(bullets));
    asteroids.erase(std::remove_if(std::begin(asteroids), std::end(asteroids), [&](Asteroid* e) { return !e; }), std::end(asteroids));
    _entities.erase(std::remove_if(std::begin(_entities) + 1, std::end(_entities), [&](std::unique_ptr<Entity>& e) { return !e; }), std::end(_entities));

    for(auto&& pending : _pending_entities) {
        _entities.emplace_back(std::move(pending));
    }
    _pending_entities.clear();
}

void Game::EndFrame_GameOver() noexcept {
    /* DO NOTHING */
}

void Game::RenderStatus() const noexcept {
    const float ui_view_height = _ui_camera.GetViewHeight();
    const float ui_view_width = ui_view_height * _ui_camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ui_view_width, ui_view_height};
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    const auto ui_leftBottom = Vector2{-ui_view_half_extents.x, ui_view_half_extents.y};
    const auto ui_rightTop = Vector2{ui_view_half_extents.x, -ui_view_half_extents.y};
    const auto ui_nearFar = Vector2{0.0f, 1.0f};
    const auto ui_cam_pos = ui_view_half_extents;
    _ui_camera.position = ui_cam_pos;
    _ui_camera.orientation_degrees = 0.0f;
    _ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, MathUtils::M_16_BY_9_RATIO);
    g_theRenderer->SetCamera(_ui_camera);

    g_theRenderer->SetModelMatrix(Matrix4::I);
    const auto* font = g_theRenderer->GetFont("System32");
    const auto font_position = ui_cam_pos - ui_view_half_extents + Vector2{5.0f, font->GetLineHeight() * 0.0f};
    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(font_position));
    g_theRenderer->DrawMultilineText(g_theRenderer->GetFont("System32"), "Score: " + std::to_string(player.GetScore()) + "\n      x" + std::to_string(player.GetLives()));

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

bool Game::IsGameOver() const noexcept {
    return player.desc.lives == 0;
}

void Game::RenderBackground() const noexcept {
    const float ui_view_height = currentGraphicsOptions.WindowHeight;
    const float ui_view_width = ui_view_height * _cameraController.GetAspectRatio();
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

void Game::StartNewWave(unsigned int wave_number) noexcept {
    for(unsigned int i = 0; i < wave_number * GetWaveMultiplierFromDifficulty(); ++i) {
        MakeLargeAsteroidOffScreen();
    }
}

void Game::DecrementLives() noexcept {
    player.DecrementLives();
}

void Game::MakeBullet(const Entity* parent, Vector2 pos, Vector2 vel) noexcept {
    auto newBullet = std::make_unique<Bullet>(parent, pos, vel);
    auto* last_entity = newBullet.get();
    _pending_entities.emplace_back(std::move(newBullet));
    auto* asBullet = reinterpret_cast<Bullet*>(last_entity);
    bullets.push_back(asBullet);
    asBullet->OnCreate();
}

void Game::MakeLargeAsteroidOffScreen() noexcept {
    const auto pos = [this]()->const Vector2 {
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

void Game::MakeLargeAsteroidAt(Vector2 pos) noexcept {
    const auto vx = MathUtils::GetRandomFloatNegOneToOne();
    const auto vy = MathUtils::GetRandomFloatNegOneToOne();
    const auto s = MathUtils::GetRandomFloatInRange(20.0f, 100.0f);
    const auto vel = Vector2{vx, vy};
    const auto rot = MathUtils::GetRandomFloatNegOneToOne() * 180.0f;
    MakeLargeAsteroid(pos, vel, rot);
}

void Game::MakeLargeAsteroidAtMouse() noexcept {
    const auto& camera = _cameraController.GetCamera();
    const auto mouseWorldCoords = g_theRenderer->ConvertScreenToWorldCoords(camera, g_theInputSystem->GetCursorScreenPosition());
    MakeLargeAsteroidAt(mouseWorldCoords);
}

void Game::MakeLargeAsteroid(Vector2 pos, Vector2 vel, float rotationSpeed) noexcept {
    SetAsteroidSpriteSheet();
    auto newAsteroid = std::make_unique<Asteroid>(Asteroid::Type::Large, pos, vel, rotationSpeed);
    AddNewAsteroidToWorld(std::move(newAsteroid));
}

void Game::AddNewAsteroidToWorld(std::unique_ptr<Asteroid> newAsteroid) {
    auto* last_entity = newAsteroid.get();
    _pending_entities.emplace_back(std::move(newAsteroid));
    auto* asAsteroid = reinterpret_cast<Asteroid*>(last_entity);
    asteroids.push_back(asAsteroid);
    asAsteroid->OnCreate();
}

void Game::SetAsteroidSpriteSheet() noexcept {
    if(!asteroid_sheet) {
        asteroid_sheet = g_theRenderer->CreateSpriteSheet("Data/Images/asteroid.png", 6, 5);
    }
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

void Game::DoCameraShake() {
    _cameraController.SetupCameraShake(currentGraphicsOptions.MaxShakeOffsetHorizontal, currentGraphicsOptions.MaxShakeOffsetVertical, currentGraphicsOptions.MaxShakeAngle);
    _cameraController.DoCameraShake([this]() { return this->GetGameOptions().cameraShakeStrength; });
}

const GameOptions& Game::GetGameOptions() const noexcept {
    return _current_options;
}

bool Game::IsEntityInView(const Entity* e) const noexcept {
    const auto cull = CalcCullBounds(_cameraController.GetCamera().GetPosition());
    return MathUtils::DoDiscsOverlap(Disc2{e->GetPosition(), e->GetCosmeticRadius()}, cull);
}

void Game::MakeShip() noexcept {
    if(!ship) {
        auto iter = _entities.begin();
        *iter = std::move(std::make_unique<Ship>(world_bounds.CalcCenter()));
        ship = reinterpret_cast<Ship*>(iter->get());
        ship->OnCreate();
    }
}

void Game::KillAll() noexcept {
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
    if(ship) {
        ship->Kill();
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

void Game::HandlePlayerInput(TimeUtils::FPSeconds deltaSeconds) {
    HandleKeyboardInput(deltaSeconds);
    HandleMouseInput(deltaSeconds);
    HandleControllerInput(deltaSeconds);
    LockCameraRotationToShip(deltaSeconds);
}

void Game::LockCameraRotationToShip(TimeUtils::FPSeconds /*deltaSeconds*/) {
    /* DO NOTHING */
}

void Game::HandleKeyboardInput(TimeUtils::FPSeconds deltaSeconds) {
    _controlling_camera = false;
    if(g_theInputSystem->IsKeyDown(KeyCode::LShift)) {
        _controlling_camera = true;
        return;
    }
    if(!_keyboard_control_active) {
        return;
    }
    if(ship) {
        if(g_theInputSystem->IsKeyDown(KeyCode::A)) {
            ship->RotateClockwise(ship->GetRotationSpeed() * deltaSeconds.count());
        } else if(g_theInputSystem->IsKeyDown(KeyCode::D)) {
            ship->RotateCounterClockwise(ship->GetRotationSpeed() * deltaSeconds.count());
        }
        if(g_theInputSystem->IsKeyDown(KeyCode::W)) {
            ship->Thrust(_thrust_force);
        }
        if(g_theInputSystem->IsKeyDown(KeyCode::Space)) {
            ship->OnFire();
        }
    }
}

void Game::HandleMouseInput(TimeUtils::FPSeconds /*deltaSeconds*/) {
    if(_controlling_camera) {
        return;
    }
    if(!_mouse_control_active) {
        return;
    }
    if(ship) {
        if(g_theInputSystem->GetMouseDelta().CalcLengthSquared() > 0.0f) {
            const auto& camera = _cameraController.GetCamera();
            auto mouseWorldCoords = g_theRenderer->ConvertScreenToWorldCoords(camera, g_theInputSystem->GetCursorScreenPosition());
            const auto newFacing = (mouseWorldCoords - ship->GetPosition()).CalcHeadingDegrees();
            ship->SetOrientationDegrees(newFacing);
        }

        if(g_theInputSystem->IsKeyDown(KeyCode::LButton)) {
            ship->OnFire();
        }
        if(g_theInputSystem->IsKeyDown(KeyCode::RButton)) {
            ship->Thrust(_thrust_force);
        }
    }
}

void Game::HandleControllerInput(TimeUtils::FPSeconds /*deltaSeconds*/) {
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
    if(asteroids.empty()) {
        StartNewWave(_current_wave++);
    }
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

void Game::OnEnter_Title() noexcept {
    /* DO NOTHING */
}

void Game::OnEnter_Options() noexcept {
    _options_selected_item = OptionsMenu::First_;
    _temp_options = _current_options;
}

void Game::OnEnter_Main() noexcept {
    explosions.clear();
    explosions.shrink_to_fit();
    asteroids.clear();
    asteroids.shrink_to_fit();
    bullets.clear();
    bullets.shrink_to_fit();
    ship = nullptr;
    _entities.clear();
    _entities.shrink_to_fit();

    _cameraController = OrthographicCameraController{g_theRenderer, g_theInputSystem};
    _cameraController.SetMaxZoomLevel(450.0f);
    _cameraController.SetZoomLevel(450.0f);

    world_bounds = AABB2::ZERO_TO_ONE;
    const auto dims = Vector2{g_theRenderer->GetOutput()->GetDimensions()};
    world_bounds.ScalePadding(dims.x, dims.y);
    world_bounds.Translate(-world_bounds.CalcCenter());

    PlayerDesc playerDesc{};
    playerDesc.lives = GetLivesFromDifficulty();
    player = Player{playerDesc};
    _current_wave = 1u;
    _entities.emplace_back(std::move(std::make_unique<Ship>(world_bounds.CalcCenter())));
    ship = reinterpret_cast<Ship*>(_entities.back().get());
}

void Game::OnEnter_GameOver() noexcept {
    /* DO NOTHING */
}

void Game::OnExit_Title() noexcept {
    /* DO NOTHING */
}

void Game::OnExit_Options() noexcept {
    /* DO NOTHING */
}

void Game::OnExit_Main() noexcept {
    /* DO NOTHING */
}

void Game::OnExit_GameOver() noexcept {
    /* DO NOTHING */
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
    g_theRenderer->DrawAABB2(world_bounds, Rgba::Green, Rgba::NoAlpha);
    g_theRenderer->DrawAABB2(CalcOrthoBounds(), Rgba::White, Rgba::NoAlpha);
    g_theRenderer->DrawAABB2(CalcViewBounds(_cameraController.GetCamera().GetPosition()), Rgba::Red, Rgba::NoAlpha);
    g_theRenderer->DrawAABB2(CalcCullBounds(_cameraController.GetCamera().GetPosition()), Rgba::White, Rgba::NoAlpha);
    g_theRenderer->DrawCircle2D(_cameraController.GetCamera().GetPosition(), 25.0f, Rgba::Pink);
}

void Game::HandleDebugInput(TimeUtils::FPSeconds deltaSeconds) {
    HandleDebugKeyboardInput(deltaSeconds);
    HandleDebugMouseInput(deltaSeconds);
}

void Game::HandleDebugKeyboardInput(TimeUtils::FPSeconds /*deltaSeconds*/) {
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
        MakeLargeAsteroidAtMouse();
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::N)) {
        MakeShip();
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::R)) {
        Respawn();
    }
}

void Game::HandleDebugMouseInput(TimeUtils::FPSeconds /*deltaSeconds*/) {
    if(g_theUISystem->GetIO().WantCaptureMouse) {
        return;
    }
}

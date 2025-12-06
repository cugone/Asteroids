#include "Game/OptionsState.hpp"

#include "Engine/Core/App.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/KerningFont.hpp"

#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"

#include "Game/TitleState.hpp"

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>

void OptionsState::OnEnter() noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        m_selected_item = OptionsMenu::First_;
        m_temp_options = game->gameOptions;
    }
    const auto* renderer = ServiceLocator::const_get<IRendererService>();
    for (const auto& mode : renderer->GetDevice()->displayModes) {
        m_validResolutions.emplace(std::make_pair(mode.width, mode.height));
    }
    m_min_resolution = m_validResolutions.begin();
    m_max_resolution = [&]() {
        auto cur_iter = m_validResolutions.begin();
        auto prev_iter = cur_iter;
        while(cur_iter != m_validResolutions.end()) {
            prev_iter = cur_iter;
            cur_iter = std::next(cur_iter);
        }
        return prev_iter;
    }();
    const auto cur_dims = renderer->GetOutput()->GetDimensions();
    std::vector<DisplayDesc> dimensions;
    dimensions.reserve(m_validResolutions.size());
    for(auto& dim : m_validResolutions) {
        dimensions.emplace_back(DisplayDesc(dim.first, dim.second));
    }
    const auto selected_dimension = renderer->GetDevice()->GetDisplayModeMatchingDimensions(dimensions, cur_dims.x, cur_dims.y);
    m_selected_resolution = [&]() {
        auto cur_iter = m_validResolutions.begin();
        if (cur_iter == m_validResolutions.end()) {
            return cur_iter;
        }
        while(cur_iter->first != selected_dimension.width && cur_iter->second != selected_dimension.height) {
            ++cur_iter;
        }
        return cur_iter;
    }();
}

void OptionsState::OnExit() noexcept {
    m_validResolutions.clear();
}

void OptionsState::BeginFrame() noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        game->SetControlType();
    }
}

void OptionsState::Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        if(auto newState = HandleInput(deltaSeconds)) {
            game->ChangeState(std::move(newState));
        }
    }
}

void OptionsState::Render() const noexcept {
    g_theRenderer->ResetModelViewProjection();
    g_theRenderer->SetRenderTargetsToBackBuffer();
    g_theRenderer->ClearDepthStencilBuffer();

    g_theRenderer->ClearColor(Rgba::Black);

    g_theRenderer->SetViewportAsPercent();

    //2D View / HUD
    const auto ui_view_height = static_cast<float>(GetGameAs<Game>()->GetSettings()->GetWindowHeight());
    const auto ui_view_width = ui_view_height * m_ui_camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ui_view_width, ui_view_height};
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    auto ui_leftBottom = Vector2{-ui_view_half_extents.x, ui_view_half_extents.y};
    auto ui_rightTop = Vector2{ui_view_half_extents.x, -ui_view_half_extents.y};
    auto ui_nearFar = Vector2{0.0f, 1.0f};
    auto ui_cam_pos = ui_view_half_extents;
    m_ui_camera.position = ui_cam_pos;
    m_ui_camera.orientation_degrees = 0.0f;
    m_ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, MathUtils::M_16_BY_9_RATIO);
    g_theRenderer->SetCamera(m_ui_camera);

    const auto* font = g_theRenderer->GetDefaultFont();
    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x, ui_view_half_extents.y * 0.25f}));
    g_theRenderer->DrawTextLine(font, "OPTIONS");

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 0.35f}));
    g_theRenderer->DrawTextLine(font, "Difficulty:", m_selected_item == OptionsMenu::DifficultySelection ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 1.5f, ui_view_half_extents.y * 0.35f}));
    g_theRenderer->DrawTextLine(font, DifficultyToString(m_temp_options.GetDifficulty()), m_selected_item == OptionsMenu::DifficultySelection ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 0.45f}));
    g_theRenderer->DrawTextLine(font, "Control Preference:", m_selected_item == OptionsMenu::ControlSelection ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 1.5f, ui_view_half_extents.y * 0.45f}));
    g_theRenderer->DrawTextLine(font, ControlPreferenceToString(m_temp_options.GetControlPreference()), m_selected_item == OptionsMenu::ControlSelection ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 0.55f}));
    g_theRenderer->DrawTextLine(font, "Camera Shake:", m_selected_item == OptionsMenu::CameraShake ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 1.5f, ui_view_half_extents.y * 0.55f}));
    {
        std::ostringstream ss;
        ss << std::setprecision(2) << m_temp_options.GetCameraShakeStrength();
        g_theRenderer->DrawTextLine(font, ss.str(), m_selected_item == OptionsMenu::CameraShake ? Rgba::Yellow : Rgba::White);
    }

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 0.65f}));
    g_theRenderer->DrawTextLine(font, "Sound Volume:", m_selected_item == OptionsMenu::SoundVolume ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 1.5f, ui_view_half_extents.y * 0.65f}));
    g_theRenderer->DrawTextLine(font, std::to_string(m_temp_options.GetSoundVolume()), m_selected_item == OptionsMenu::SoundVolume ? Rgba::Yellow : Rgba::White);
    
    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 0.75f}));
    g_theRenderer->DrawTextLine(font, "Music Volume:", m_selected_item == OptionsMenu::MusicVolume ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 1.5f, ui_view_half_extents.y * 0.75f}));
    g_theRenderer->DrawTextLine(font, std::to_string(m_temp_options.GetMusicVolume()), m_selected_item == OptionsMenu::MusicVolume ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 0.85f }));
    g_theRenderer->DrawTextLine(font, "Display Mode:", m_selected_item == OptionsMenu::DisplayMode ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ ui_view_half_extents.x * 1.5f, ui_view_half_extents.y * 0.85f }));
    g_theRenderer->DrawTextLine(font, DisplayModePreferenceToString(), m_selected_item == OptionsMenu::DisplayMode ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 0.95f }));
    g_theRenderer->DrawTextLine(font, "Resolution:", GetResolutionItemColor());

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ ui_view_half_extents.x * 1.5f, ui_view_half_extents.y * 0.95f }));
    g_theRenderer->DrawTextLine(font, StringUtils::to_string(m_temp_options.GetWindowResolution()), GetResolutionItemColor());


    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 1.05f}));
    g_theRenderer->DrawTextLine(font, "Back", m_selected_item == OptionsMenu::Cancel ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 1.15f}));
    g_theRenderer->DrawTextLine(font, "Accept", m_selected_item == OptionsMenu::Accept ? Rgba::Yellow : Rgba::White);

}

void OptionsState::EndFrame() noexcept {
    /* DO NOTHING */
}

std::unique_ptr<GameState> OptionsState::HandleInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(auto kb_state = HandleKeyboardInput()) {
        return kb_state;
    }
    if(auto ctlr_state = HandleControllerInput()) {
        return ctlr_state;
    }
    return {};
}

std::unique_ptr<GameState> OptionsState::HandleKeyboardInput() noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        if(!game->IsKeyboardActive()) {
            return {};
        }
    } else {
        return {};
    }
    const bool up = g_theInputSystem->WasKeyJustPressed(KeyCode::W) || g_theInputSystem->WasKeyJustPressed(KeyCode::Up);
    const bool down = g_theInputSystem->WasKeyJustPressed(KeyCode::S) || g_theInputSystem->WasKeyJustPressed(KeyCode::Down);
    const bool left = g_theInputSystem->WasKeyJustPressed(KeyCode::A) || g_theInputSystem->WasKeyJustPressed(KeyCode::Left);
    const bool right = g_theInputSystem->WasKeyJustPressed(KeyCode::D) || g_theInputSystem->WasKeyJustPressed(KeyCode::Right);
    const bool select = g_theInputSystem->WasKeyJustPressed(KeyCode::Enter) || g_theInputSystem->WasKeyJustPressed(KeyCode::NumPadEnter);
    const bool cancel = g_theInputSystem->WasKeyJustPressed(KeyCode::Esc);
    return HandleOptionsMenuState(up, down, left, right, cancel, select);
}

std::unique_ptr<GameState> OptionsState::HandleControllerInput() noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        if(const auto& controller = g_theInputSystem->GetXboxController(0); game->IsControllerActive() && controller.IsConnected()) {
            const bool up = controller.WasButtonJustPressed(XboxController::Button::Up);
            const bool down = controller.WasButtonJustPressed(XboxController::Button::Down);
            const bool left = controller.WasButtonJustPressed(XboxController::Button::Left);
            const bool right = controller.WasButtonJustPressed(XboxController::Button::Right);
            const bool select = controller.WasButtonJustPressed(XboxController::Button::A);
            const bool cancel = controller.WasButtonJustPressed(XboxController::Button::B) || controller.WasButtonJustPressed(XboxController::Button::Back);
            return HandleOptionsMenuState(up, down, left, right, cancel, select);
        }
    }
    return {};
}

std::unique_ptr<GameState> OptionsState::HandleOptionsMenuState(const bool up, const bool down, const bool left, const bool right, const bool cancel, const bool select) noexcept {
    if(up) {
        --m_selected_item;
        m_selected_item = std::clamp(m_selected_item, OptionsMenu::First_, OptionsMenu::Last_Valid_);
    } else if(down) {
        ++m_selected_item;
        m_selected_item = std::clamp(m_selected_item, OptionsMenu::First_, OptionsMenu::Last_Valid_);
    }
    if(left) {
        CycleSelectedOptionDown(m_selected_item);
    } else if(right) {
        CycleSelectedOptionUp(m_selected_item);
    }
    if(cancel) {
        return std::make_unique<TitleState>();
    }
    if(select) {
        switch(m_selected_item) {
        case OptionsMenu::Cancel: return std::make_unique<TitleState>();
        case OptionsMenu::Accept:
            if(TrySetResolution()) {
                SaveCurrentOptions();
                return std::make_unique<TitleState>();
            }
            break;
        default: return {};
        }
    }
    return {};
}

void OptionsState::CycleSelectedOptionDown(OptionsMenu selectedItem) noexcept {
    switch(selectedItem) {
    case OptionsMenu::DifficultySelection:
    {
        auto cur_difficulty = m_temp_options.GetDifficulty();
        --cur_difficulty;
        cur_difficulty = std::clamp(cur_difficulty, Difficulty::First_, Difficulty::Last_Valid_);
        m_temp_options.SetDifficulty(cur_difficulty);
        break;
    }
    case OptionsMenu::ControlSelection:
    {
        auto cur_pref = m_temp_options.GetControlPreference();
        --cur_pref;
        cur_pref = std::clamp(cur_pref, ControlPreference::First_, ControlPreference::Last_Valid_);
        m_temp_options.SetControlPreference(cur_pref);
        break;
    }
    case OptionsMenu::SoundVolume:
    {
        auto cur_soundVolume = m_temp_options.GetSoundVolume();
        m_temp_options.SetSoundVolume(std::clamp(cur_soundVolume ? --cur_soundVolume : cur_soundVolume, m_min_sound_volume, m_max_sound_volume));
        AudioSystem::SoundDesc desc{};
        desc.groupName = g_audiogroup_sound;
        g_theAudioSystem->Play(g_sound_shootpath, desc);
        break;
    }
    case OptionsMenu::MusicVolume:
    {
        auto cur_musicVolume = m_temp_options.GetMusicVolume();
        m_temp_options.SetMusicVolume(std::clamp(cur_musicVolume ? --cur_musicVolume : cur_musicVolume, m_min_music_volume, m_max_music_volume));
        break;
    }
    case OptionsMenu::DisplayMode:
    {
        if(auto is_windowed = m_temp_options.IsWindowed(); is_windowed) {
            m_temp_options.SetFullscreenMode();
        } else {
            m_temp_options.SetWindowedMode();
        }
        break;
    }
    case OptionsMenu::Resolution:
    {
        if (auto is_fullscreen = m_temp_options.IsWindowed()) {
            m_selected_resolution = [&]() {
                if (m_selected_resolution == m_min_resolution) {
                    return m_selected_resolution;
                }
                else {
                    return std::prev(m_selected_resolution);
                }
                }();
            m_temp_options.SetWindowResolution(IntVector2(m_selected_resolution->first, m_selected_resolution->second));
        }
        break;
    }
    case OptionsMenu::CameraShake:
    {
        auto cur_cameraShake = m_temp_options.GetCameraShakeStrength();
        cur_cameraShake -= 0.1f;
        m_temp_options.SetCameraShakeStrength(std::clamp(cur_cameraShake, m_min_camera_shake, m_max_camera_shake));
        break;
    }
    default:
        /* DO NOTHING */
        break;
    }
}

void OptionsState::CycleSelectedOptionUp(OptionsMenu selectedItem) noexcept {
    switch(selectedItem) {
    case OptionsMenu::DifficultySelection:
    {
        auto cur_difficulty = m_temp_options.GetDifficulty();
        ++cur_difficulty;
        cur_difficulty = std::clamp(cur_difficulty, Difficulty::First_, Difficulty::Last_Valid_);
        m_temp_options.SetDifficulty(cur_difficulty);
        break;
    }
    case OptionsMenu::ControlSelection:
    {
        auto cur_pref = m_temp_options.GetControlPreference();
        ++cur_pref;
        cur_pref = std::clamp(cur_pref, ControlPreference::First_, ControlPreference::Last_Valid_);
        m_temp_options.SetControlPreference(cur_pref);
        break;
    }
    case OptionsMenu::SoundVolume:
    {
        auto cur_soundVolume = m_temp_options.GetSoundVolume();
        m_temp_options.SetSoundVolume((std::min)(++cur_soundVolume, m_max_sound_volume));
        AudioSystem::SoundDesc desc{};
        desc.groupName = g_audiogroup_sound;
        g_theAudioSystem->Play(g_sound_shootpath, desc);
        break;
    }
    case OptionsMenu::MusicVolume:
    {
        auto cur_musicVolume = m_temp_options.GetMusicVolume();
        m_temp_options.SetMusicVolume((std::min)(++cur_musicVolume, m_max_music_volume));
        break;
    }
    case OptionsMenu::DisplayMode:
    {
        if (auto is_windowed = m_temp_options.IsWindowed(); is_windowed) {
            m_temp_options.SetFullscreenMode();
        } else {
            m_temp_options.SetWindowedMode();
        }
        break;
    }
    case OptionsMenu::Resolution:
    {
        if (auto is_fullscreen = m_temp_options.IsWindowed()) {
            m_selected_resolution = [&]() {
                if (m_selected_resolution == m_max_resolution) {
                    return m_selected_resolution;
                }
                else {
                    return std::next(m_selected_resolution);
                }
                }();
            m_temp_options.SetWindowResolution(IntVector2(m_selected_resolution->first, m_selected_resolution->second));
        }
        break;
    }
    case OptionsMenu::CameraShake:
    {
        auto cur_cameraShake = m_temp_options.GetCameraShakeStrength();
        cur_cameraShake += 0.1f;
        m_temp_options.SetCameraShakeStrength(std::clamp(cur_cameraShake, m_min_camera_shake, m_max_camera_shake));
        break;
    }
    default:
        /* DO NOTHING */
        break;
    }
}

void OptionsState::SaveCurrentOptions() noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        game->gameOptions = m_temp_options;
        game->gameOptions.SaveToConfig(*g_theConfig);
        std::ofstream ofs(g_options_filepath);
        g_theConfig->PrintConfigs(ofs);
        ofs.flush();
        ofs.close();
    }
}

void OptionsState::SetOptionsToDefault() noexcept {
    if(auto* game = GetGameAs<Game>(); game != nullptr) {
        game->gameOptions.SetToDefault();
    }
}

std::string OptionsState::DifficultyToString(Difficulty difficulty) const noexcept {
    switch(difficulty) {
    case Difficulty::Easy: return "Easy";
    case Difficulty::Normal: return "Normal";
    case Difficulty::Hard: return "Hard";
    default: return "";
    }
}

std::string OptionsState::ControlPreferenceToString(ControlPreference preference) const noexcept {
    switch(preference) {
    case ControlPreference::Keyboard: return "Keyboard";
    case ControlPreference::Mouse: return "Mouse";
    case ControlPreference::XboxController: return "Xbox Controller";
    default: return "";
    }
}

std::string OptionsState::DisplayModePreferenceToString() const noexcept {
    if (m_temp_options.IsFullscreen()) {
        return "Borderless Fullscreen";
    } else if (m_temp_options.IsWindowed()) {
        return "Windowed";
    }
    return "";
}

Rgba OptionsState::GetResolutionItemColor() const noexcept {
    auto selected_color = Rgba::White;
    if(m_temp_options.IsFullscreen()) {
        selected_color = Rgba::Grey;
        if(m_selected_item == OptionsMenu::Resolution) {
            selected_color = Rgba::LightGrey;
        }
    } else {
        if (m_selected_item == OptionsMenu::Resolution) {
            selected_color = Rgba::Yellow;
        }
    }
    return selected_color;
}

bool OptionsState::TrySetResolution() noexcept {
    if(m_temp_options.IsFullscreen()) {
        if (auto* app = ServiceLocator::get<IAppService>(); app != nullptr) {
            app->Maximize();
            return true;
        }
        return false;
    } else if (m_temp_options.IsWindowed()) {
        if (auto* app = ServiceLocator::get<IAppService>(); app != nullptr) {
            app->Restore(m_selected_resolution->first, m_selected_resolution->second);
            return true;
        }
        return false;
    }
    return false;
}

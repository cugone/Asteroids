#include "Game/OptionsState.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/KerningFont.hpp"

#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Renderer/Renderer.hpp"

#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"

#include "Game/TitleState.hpp"

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>

void OptionsState::OnEnter() noexcept {
    m_selected_item = OptionsMenu::First_;
    m_temp_options = g_theGame->gameOptions;
}

void OptionsState::OnExit() noexcept {
    /* DO NOTHING */
}

void OptionsState::BeginFrame() noexcept {
    g_theGame->SetControlType();
}

void OptionsState::Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) {
    if(auto newState = HandleInput(deltaSeconds)) {
        g_theGame->ChangeState(std::move(newState));
    }
}

void OptionsState::Render() const noexcept {
    g_theRenderer->ResetModelViewProjection();
    g_theRenderer->SetRenderTargetsToBackBuffer();
    g_theRenderer->ClearDepthStencilBuffer();

    g_theRenderer->ClearColor(Rgba::Black);

    g_theRenderer->SetViewportAsPercent();

    //2D View / HUD
    const float ui_view_height = currentGraphicsOptions.WindowHeight;
    const float ui_view_width = ui_view_height * m_ui_camera.GetAspectRatio();
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

    const auto* font = g_theRenderer->GetFont("System32");
    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x, ui_view_half_extents.y * 0.25f}));
    g_theRenderer->DrawTextLine(font, "OPTIONS");

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 0.35f}));
    g_theRenderer->DrawTextLine(font, "Difficulty:", m_selected_item == OptionsMenu::DifficultySelection ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 1.5f, ui_view_half_extents.y * 0.35f}));
    g_theRenderer->DrawTextLine(font, DifficultyToString(m_temp_options.difficulty), m_selected_item == OptionsMenu::DifficultySelection ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 0.45f}));
    g_theRenderer->DrawTextLine(font, "Control Preference:", m_selected_item == OptionsMenu::ControlSelection ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 1.5f, ui_view_half_extents.y * 0.45f}));
    g_theRenderer->DrawTextLine(font, ControlPreferenceToString(m_temp_options.controlPref), m_selected_item == OptionsMenu::ControlSelection ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 0.55f}));
    g_theRenderer->DrawTextLine(font, "Camera Shake:", m_selected_item == OptionsMenu::CameraShake ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 1.5f, ui_view_half_extents.y * 0.55f}));
    {
        std::ostringstream ss;
        ss << std::setprecision(2) << m_temp_options.cameraShakeStrength;
        g_theRenderer->DrawTextLine(font, ss.str(), m_selected_item == OptionsMenu::CameraShake ? Rgba::Yellow : Rgba::White);
    }

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 0.65f}));
    g_theRenderer->DrawTextLine(font, "Sound Volume:", m_selected_item == OptionsMenu::SoundVolume ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 1.5f, ui_view_half_extents.y * 0.65f}));
    g_theRenderer->DrawTextLine(font, std::to_string(m_temp_options.soundVolume), m_selected_item == OptionsMenu::SoundVolume ? Rgba::Yellow : Rgba::White);
    
    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 0.75f}));
    g_theRenderer->DrawTextLine(font, "Music Volume:", m_selected_item == OptionsMenu::MusicVolume ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 1.5f, ui_view_half_extents.y * 0.75f}));
    g_theRenderer->DrawTextLine(font, std::to_string(m_temp_options.musicVolume), m_selected_item == OptionsMenu::MusicVolume ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 0.95f}));
    g_theRenderer->DrawTextLine(font, "Back", m_selected_item == OptionsMenu::Cancel ? Rgba::Yellow : Rgba::White);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{ui_view_half_extents.x * 0.25f, ui_view_half_extents.y * 1.05f}));
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
    if(!g_theGame->IsKeyboardActive()) {
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
    if(const auto controller = g_theInputSystem->GetXboxController(0); g_theGame->IsControllerActive() && controller.IsConnected()) {
        const bool up = controller.WasButtonJustPressed(XboxController::Button::Up) || MathUtils::IsEquivalent(1.0f, controller.GetLeftThumbPosition().y);
        const bool down = controller.WasButtonJustPressed(XboxController::Button::Down) || MathUtils::IsEquivalent(-1.0f, controller.GetLeftThumbPosition().y);
        const bool left = controller.WasButtonJustPressed(XboxController::Button::Left) || MathUtils::IsEquivalent(-1.0f, controller.GetLeftThumbPosition().x);
        const bool right = controller.WasButtonJustPressed(XboxController::Button::Right) || MathUtils::IsEquivalent(1.0f, controller.GetLeftThumbPosition().x);
        const bool select = controller.WasButtonJustPressed(XboxController::Button::A);
        const bool cancel = controller.WasButtonJustPressed(XboxController::Button::B);
        return HandleOptionsMenuState(up, down, left, right, cancel, select);
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
            SaveCurrentOptions();
            return std::make_unique<TitleState>(); 
        default: return {};
        }
    }
    return {};
}

void OptionsState::CycleSelectedOptionDown(OptionsMenu selectedItem) noexcept {
    switch(selectedItem) {
    case OptionsMenu::DifficultySelection:
    {
        if(m_temp_options.difficulty == Difficulty::First_) {
            m_temp_options.difficulty = Difficulty::Last_;
        }
        --m_temp_options.difficulty;
        break;
    }
    case OptionsMenu::ControlSelection:
    {
        if(m_temp_options.controlPref == ControlPreference::First_) {
            m_temp_options.controlPref = ControlPreference::Last_;
        }
        --m_temp_options.controlPref;
        break;
    }
    case OptionsMenu::SoundVolume:
    {
        --m_temp_options.soundVolume;
        m_temp_options.soundVolume = std::clamp(m_temp_options.soundVolume, m_min_sound_volume, m_max_sound_volume);
        auto* group = g_theAudioSystem->GetChannelGroup(g_audiogroup_sound);
        const float volumeAsFloat = m_temp_options.soundVolume / static_cast<float>(m_max_sound_volume);
        group->SetVolume(volumeAsFloat);
        g_theAudioSystem->Play(g_sound_shootpath);
        break;
    }
    case OptionsMenu::MusicVolume:
    {
        --m_temp_options.musicVolume;
        m_temp_options.musicVolume = std::clamp(m_temp_options.musicVolume, m_min_music_volume, m_max_music_volume);
        auto* group = g_theAudioSystem->GetChannelGroup(g_audiogroup_music);
        const float volumeAsFloat = m_temp_options.musicVolume / static_cast<float>(m_max_music_volume);
        group->SetVolume(volumeAsFloat);
        break;
    }
    case OptionsMenu::CameraShake:
        m_temp_options.cameraShakeStrength -= 0.1f;
        m_temp_options.cameraShakeStrength = std::clamp(m_temp_options.cameraShakeStrength, m_min_camera_shake, m_max_camera_shake);
        break;
    default: /* DO NOTHING */;
    }
}

void OptionsState::CycleSelectedOptionUp(OptionsMenu selectedItem) noexcept {
    switch(selectedItem) {
    case OptionsMenu::DifficultySelection:
    {
        ++m_temp_options.difficulty;
        if(m_temp_options.difficulty == Difficulty::Last_) {
            m_temp_options.difficulty = Difficulty::First_;
        }
        break;
    }
    case OptionsMenu::ControlSelection:
    {
        ++m_temp_options.controlPref;
        if(m_temp_options.controlPref == ControlPreference::Last_) {
            m_temp_options.controlPref = ControlPreference::First_;
        }
        break;
    }
    case OptionsMenu::SoundVolume:
    {
        ++m_temp_options.soundVolume;
        m_temp_options.soundVolume = std::clamp(m_temp_options.soundVolume, m_min_sound_volume, m_max_sound_volume);
        auto* group = g_theAudioSystem->GetChannelGroup(g_audiogroup_sound);
        const float volumeAsFloat = m_temp_options.soundVolume / static_cast<float>(m_max_sound_volume);
        group->SetVolume(volumeAsFloat);
        g_theAudioSystem->Play(g_sound_shootpath);
        break;
    }
    case OptionsMenu::MusicVolume:
    {
        ++m_temp_options.musicVolume;
        m_temp_options.musicVolume = std::clamp(m_temp_options.musicVolume, m_min_music_volume, m_max_music_volume);
        auto* group = g_theAudioSystem->GetChannelGroup(g_audiogroup_music);
        const float volumeAsFloat = m_temp_options.musicVolume / static_cast<float>(m_max_music_volume);
        group->SetVolume(volumeAsFloat);
        g_theAudioSystem->Play(g_music_bgmpath);
        break;
    }
    case OptionsMenu::CameraShake:
        m_temp_options.cameraShakeStrength += 0.1f;
        m_temp_options.cameraShakeStrength = std::clamp(m_temp_options.cameraShakeStrength, m_min_camera_shake, m_max_camera_shake);
        break;
    default: /* DO NOTHING */;
    }
}

void OptionsState::SaveCurrentOptions() noexcept {
    g_theGame->gameOptions = m_temp_options;
    g_theConfig->SetValue("difficulty", DifficultyToString(g_theGame->gameOptions.difficulty));
    g_theConfig->SetValue("controlpref", ControlPreferenceToString(g_theGame->gameOptions.controlPref));
    g_theConfig->SetValue("sound", static_cast<int>(g_theGame->gameOptions.soundVolume));
    g_theConfig->SetValue("music", static_cast<int>(g_theGame->gameOptions.musicVolume));
    g_theConfig->SetValue("cameraShakeStrength", g_theGame->gameOptions.cameraShakeStrength);
    std::ofstream ofs(g_options_filepath);
    ofs << *g_theConfig;
    ofs.close();
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

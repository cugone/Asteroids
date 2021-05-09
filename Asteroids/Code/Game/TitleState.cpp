#include "Game/TitleState.hpp"

#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/KerningFont.hpp"

#include "Engine/Renderer/Renderer.hpp"

#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"

#include "Game/MainState.hpp"
#include "Game/OptionsState.hpp"

void TitleState::OnEnter() noexcept {
    /* DO NOTHING */
}

void TitleState::OnExit() noexcept {
    /* DO NOTHING */
}

void TitleState::BeginFrame() noexcept {
    g_theGame->SetControlType();
}

void TitleState::Update([[maybe_unused]] a2de::TimeUtils::FPSeconds deltaSeconds) {
    if(auto newState = HandleInput(deltaSeconds)) {
        g_theGame->ChangeState(std::move(newState));
    }
}

void TitleState::Render() const noexcept {
    g_theRenderer->ResetModelViewProjection();
    g_theRenderer->SetRenderTargetsToBackBuffer();
    g_theRenderer->ClearDepthStencilBuffer();

    g_theRenderer->ClearColor(a2de::Rgba::Black);

    g_theRenderer->SetViewportAsPercent();

    //2D View / HUD
    const float ui_view_height = currentGraphicsOptions.WindowHeight;
    const float ui_view_width = ui_view_height * m_ui_camera.GetAspectRatio();
    const auto ui_view_extents = a2de::Vector2{ui_view_width, ui_view_height};
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    const auto ui_leftBottom = a2de::Vector2{-ui_view_half_extents.x, ui_view_half_extents.y};
    const auto ui_rightTop = a2de::Vector2{ui_view_half_extents.x, -ui_view_half_extents.y};
    const auto ui_nearFar = a2de::Vector2{0.0f, 1.0f};
    const auto ui_cam_pos = ui_view_half_extents;
    m_ui_camera.position = ui_cam_pos;
    m_ui_camera.orientation_degrees = 0.0f;
    m_ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, a2de::MathUtils::M_16_BY_9_RATIO);
    g_theRenderer->SetCamera(m_ui_camera);

    const auto* font = g_theRenderer->GetFont("System32");
    const auto line_height = font->GetLineHeight();

    g_theRenderer->SetModelMatrix(a2de::Matrix4::CreateTranslationMatrix(a2de::Vector2{ui_view_half_extents.x, ui_view_half_extents.y + line_height * 0.0f}));
    g_theRenderer->DrawTextLine(font, "ASTEROIDS");

    g_theRenderer->SetModelMatrix(a2de::Matrix4::CreateTranslationMatrix(a2de::Vector2{ui_view_half_extents.x, ui_view_half_extents.y + line_height * 2.0f}));
    g_theRenderer->DrawTextLine(font, "START", m_selected_item == TitleMenu::Start ? a2de::Rgba::Yellow : a2de::Rgba::White);

    g_theRenderer->SetModelMatrix(a2de::Matrix4::CreateTranslationMatrix(a2de::Vector2{ui_view_half_extents.x, ui_view_half_extents.y + line_height * 4.0f}));
    g_theRenderer->DrawTextLine(font, "OPTIONS", m_selected_item == TitleMenu::Options ? a2de::Rgba::Yellow : a2de::Rgba::White);

    g_theRenderer->SetModelMatrix(a2de::Matrix4::CreateTranslationMatrix(a2de::Vector2{ui_view_half_extents.x, ui_view_half_extents.y + line_height * 6.0f}));
    g_theRenderer->DrawTextLine(font, "EXIT", m_selected_item == TitleMenu::Exit ? a2de::Rgba::Yellow : a2de::Rgba::White);

}

void TitleState::EndFrame() noexcept {
    /* DO NOTHING */
}

std::unique_ptr<GameState> TitleState::HandleInput([[maybe_unused]] a2de::TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(auto kb_state = HandleKeyboardInput()) {
        return kb_state;
    }
    if(auto ctlr_state = HandleControllerInput()) {
        return ctlr_state;
    }
    return {};
}

std::unique_ptr<GameState> TitleState::HandleKeyboardInput() noexcept {
    if(!g_theGame->IsKeyboardActive()) {
        return {};
    }
    const bool up = g_theInputSystem->WasKeyJustPressed(a2de::KeyCode::W) || g_theInputSystem->WasKeyJustPressed(a2de::KeyCode::Up);
    const bool down = g_theInputSystem->WasKeyJustPressed(a2de::KeyCode::S) || g_theInputSystem->WasKeyJustPressed(a2de::KeyCode::Down);
    const bool select = g_theInputSystem->WasKeyJustPressed(a2de::KeyCode::Enter) || g_theInputSystem->WasKeyJustPressed(a2de::KeyCode::NumPadEnter);
    const bool cancel = g_theInputSystem->WasKeyJustPressed(a2de::KeyCode::Esc);
    if(up) {
        --m_selected_item;
        m_selected_item = std::clamp(m_selected_item, TitleMenu::First_, TitleMenu::Last_Valid_);
    } else if(down) {
        ++m_selected_item;
        m_selected_item = std::clamp(m_selected_item, TitleMenu::First_, TitleMenu::Last_Valid_);
    }
    if(cancel) {
        m_selected_item = TitleMenu::Exit;
    }
    if(select) {
        switch(m_selected_item) {
        case TitleMenu::Start:
            return std::make_unique<MainState>();
        case TitleMenu::Options:
            return std::make_unique<OptionsState>();
        case TitleMenu::Exit:
            g_theApp->SetIsQuitting(true);
            return {};
        default:
            ERROR_AND_DIE("TITLE MENU ENUM HAS CHANGED.");
        }
    }
    return {};
}

std::unique_ptr<GameState> TitleState::HandleControllerInput() noexcept {
    if(const auto controller = g_theInputSystem->GetXboxController(0); g_theGame->IsControllerActive() && controller.IsConnected()) {
        const bool up = controller.WasButtonJustPressed(a2de::XboxController::Button::Up) || a2de::MathUtils::IsEquivalent(1.0f, controller.GetLeftThumbPosition().y);
        const bool down = controller.WasButtonJustPressed(a2de::XboxController::Button::Down) || a2de::MathUtils::IsEquivalent(-1.0f, controller.GetLeftThumbPosition().y);
        const bool select = g_theInputSystem->WasKeyJustPressed(a2de::KeyCode::Enter) || g_theInputSystem->WasKeyJustPressed(a2de::KeyCode::NumPadEnter);
        const bool cancel = g_theInputSystem->WasKeyJustPressed(a2de::KeyCode::Esc);
        if(up) {
            --m_selected_item;

        } else if(down) {
            ++m_selected_item;
        }
        if(cancel) {
            m_selected_item = TitleMenu::Exit;
        }
        if(select) {
            switch(m_selected_item) {
            case TitleMenu::Start:
                return std::make_unique<MainState>();
            case TitleMenu::Options:
                return std::make_unique<OptionsState>();
            case TitleMenu::Exit:
                g_theApp->SetIsQuitting(true);
                return {};
            default:
                ERROR_AND_DIE("TITLE MENU ENUM HAS CHANGED.");
            }
        }
    }
    return {};
}

#include "Game/GameOverState.hpp"

#include "Engine/Input/InputSystem.hpp"

#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"
#include "Game/TitleState.hpp"

void GameOverState::OnEnter() noexcept {
    /* DO NOTHING */
}

void GameOverState::OnExit() noexcept {
    /* DO NOTHING */
}

void GameOverState::BeginFrame() noexcept {
    g_theGame->SetControlType();
}

void GameOverState::Update([[maybe_unused]] a2de::TimeUtils::FPSeconds deltaSeconds) {
    if(auto newState = HandleInput(deltaSeconds)) {
        g_theGame->ChangeState(std::move(newState));
    }
}

std::unique_ptr<GameState> GameOverState::HandleInput([[maybe_unused]] a2de::TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(auto kb_state = HandleKeyboardInput()) {
        return kb_state;
    }
    if(auto ctlr_state = HandleControllerInput()) {
        return ctlr_state;
    }
    if(auto mouse_state = HandleMouseInput()) {
        return mouse_state;
    }
    return {};
}

std::unique_ptr<GameState> GameOverState::HandleKeyboardInput() noexcept {
    if(g_theInputSystem->WasAnyKeyPressed()) {
        return std::make_unique<TitleState>();
    }
    return {};
}

std::unique_ptr<GameState> GameOverState::HandleControllerInput() noexcept {
    if(const auto controller = g_theInputSystem->GetXboxController(0); controller.IsConnected() && controller.WasAnyButtonJustPressed()) {
        return std::make_unique<TitleState>();
    }
    return {};
}

std::unique_ptr<GameState> GameOverState::HandleMouseInput() noexcept {
    if(g_theInputSystem->WasAnyMouseButtonPressed()) {
        return std::make_unique<TitleState>();
    }
    return {};
}

void GameOverState::Render() const noexcept {
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
    auto ui_leftBottom = a2de::Vector2{-ui_view_half_extents.x, ui_view_half_extents.y};
    auto ui_rightTop = a2de::Vector2{ui_view_half_extents.x, -ui_view_half_extents.y};
    auto ui_nearFar = a2de::Vector2{0.0f, 1.0f};
    auto ui_cam_pos = ui_view_half_extents;
    m_ui_camera.position = ui_cam_pos;
    m_ui_camera.orientation_degrees = 0.0f;
    m_ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, a2de::MathUtils::M_16_BY_9_RATIO);
    g_theRenderer->SetCamera(m_ui_camera);

    const auto* font = g_theRenderer->GetFont("System32");
    g_theRenderer->SetModelMatrix(a2de::Matrix4::CreateTranslationMatrix(ui_view_half_extents));
    g_theRenderer->DrawTextLine(font, "GAME OVER");

}

void GameOverState::EndFrame() noexcept {
    /* DO NOTHING */
}

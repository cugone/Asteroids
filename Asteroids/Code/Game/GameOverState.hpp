#pragma once

#include "Engine/Core/TypeUtils.hpp"

#include "Engine/Renderer/Camera2D.hpp"

#include "Game/GameState.hpp"

#include <memory>

class GameOverState : public GameState {
public:
    virtual ~GameOverState() = default;

    void OnEnter() noexcept override;
    void OnExit() noexcept override;
    void BeginFrame() noexcept override;
    void Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) override;
    void Render() const noexcept override;
    void EndFrame() noexcept override;

protected:
private:
    std::unique_ptr<GameState> HandleInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept override;
    std::unique_ptr<GameState> HandleKeyboardInput() noexcept;
    std::unique_ptr<GameState> HandleControllerInput() noexcept;
    std::unique_ptr<GameState> HandleMouseInput() noexcept;

    mutable Camera2D m_ui_camera{};
};

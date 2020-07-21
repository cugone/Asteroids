#pragma once

#include "Engine/Core/TypeUtils.hpp"

#include "Engine/Renderer/Camera2D.hpp"

#include "Game/GameState.hpp"

#include <memory>

enum class TitleMenu {
    First_,
    Start = First_,
    Options,
    Last_Valid_,
    Exit = Last_Valid_,
    Last_,
};

template<>
struct TypeUtils::is_incrementable_enum_type<TitleMenu> : std::true_type {};

template<>
struct TypeUtils::is_decrementable_enum_type<TitleMenu> : std::true_type {};

class TitleState : public GameState {
public:
    virtual ~TitleState() = default;

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

    mutable Camera2D m_ui_camera{};
    TitleMenu m_selected_item{TitleMenu::First_};
};

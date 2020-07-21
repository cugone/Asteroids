#pragma once

#include "Engine/Core/TypeUtils.hpp"

#include "Engine/Renderer/Camera2D.hpp"

#include "Game/Game.hpp"
#include "Game/GameState.hpp"

#include <memory>

enum class OptionsMenu {
    First_,
    DifficultySelection = First_,
    ControlSelection,
    CameraShake,
    SoundVolume,
    MusicVolume,
    Cancel,
    Last_Valid_,
    Accept = Last_Valid_,
    Last_
};

template<>
struct TypeUtils::is_incrementable_enum_type<OptionsMenu> : std::true_type {};

template<>
struct TypeUtils::is_decrementable_enum_type<OptionsMenu> : std::true_type {};

class OptionsState : public GameState {
public:
    virtual ~OptionsState() = default;

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

    std::unique_ptr<GameState> HandleOptionsMenuState(const bool up, const bool down, const bool left, const bool right, const bool cancel, const bool select) noexcept;

    void CycleSelectedOptionDown(OptionsMenu selectedItem) noexcept;
    void CycleSelectedOptionUp(OptionsMenu selectedItem) noexcept;
    void SaveCurrentOptions();
    std::string DifficultyToString(Difficulty difficulty) const noexcept;
    std::string ControlPreferenceToString(ControlPreference preference) const noexcept;

    mutable Camera2D m_ui_camera{};
    OptionsMenu m_selected_item{OptionsMenu::First_};
    GameOptions m_temp_options{};
    float m_min_camera_shake{0.0f};
    float m_max_camera_shake{1.0f};
    uint8_t m_max_sound_volume{10u};
    uint8_t m_min_sound_volume{0u};
    uint8_t m_max_music_volume{10u};
    uint8_t m_min_music_volume{0u};

};

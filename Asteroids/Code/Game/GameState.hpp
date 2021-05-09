#pragma once

#include "Engine/Core/TimeUtils.hpp"

#include <memory>

class GameState {
public:
    virtual ~GameState() = 0;
    virtual void OnEnter() noexcept = 0;
    virtual void OnExit() noexcept = 0;
    virtual void BeginFrame() noexcept = 0;
    virtual void Update([[maybe_unused]] a2de::TimeUtils::FPSeconds deltaSeconds) = 0;
    virtual void Render() const noexcept = 0;
    virtual void EndFrame() noexcept = 0;
protected:
    virtual std::unique_ptr<GameState> HandleInput([[maybe_unused]] a2de::TimeUtils::FPSeconds deltaSeconds) noexcept = 0;
private:
};

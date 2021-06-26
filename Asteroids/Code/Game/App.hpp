#pragma once

#include "Engine/Core/Config.hpp"
#include "Engine/Core/EngineSubsystem.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include <memory>

class App : public EngineSubsystem {
public:
    App(const std::string& cmdString);
    App(const App& other) = default;
    App(App&& other) = default;
    App& operator=(const App& other) = default;
    App& operator=(App&& other) = default;
    virtual ~App();

    virtual void Initialize() override;
    void RunFrame();

    bool IsQuitting() const;
    void SetIsQuitting(bool value);

    bool HasFocus() const;
    bool LostFocus() const;
    bool GainedFocus() const;

protected:
private:
    void SetupEngineSystemPointers();
    void SetupEngineSystemChainOfResponsibility();

    virtual void BeginFrame() override;
    virtual void Update(TimeUtils::FPSeconds deltaSeconds) override;
    virtual void Render() const override;
    virtual void EndFrame() override;
    virtual bool ProcessSystemMessage(const EngineMessage& msg) noexcept override;
    void LogSystemDescription() const;

    bool _isQuitting = false;
    bool _current_focus = false;
    bool _previous_focus = false;

    std::unique_ptr<class Config> _theConfig{};
    std::unique_ptr<class JobSystem> _theJobSystem{};
    std::unique_ptr<class FileLogger> _theFileLogger{};
    std::unique_ptr<class Renderer> _theRenderer{};
    std::unique_ptr<class InputSystem> _theInputSystem{};
    std::unique_ptr<class AudioSystem> _theAudioSystem{};
    std::unique_ptr<class UISystem> _theUI{};
    std::unique_ptr<class Console> _theConsole{};
    std::unique_ptr<class Game> _theGame{};

};

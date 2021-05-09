#pragma once

namespace a2de {
    class JobSystem;
    class FileLogger;
    class Renderer;
    class Console;
    class Config;
    class UISystem;
    class InputSystem;
    class AudioSystem;
    class EngineSubsystem;
}

class App;
class Game;

extern a2de::JobSystem* g_theJobSystem;
extern a2de::FileLogger* g_theFileLogger;
extern a2de::Renderer* g_theRenderer;
extern a2de::Console* g_theConsole;
extern a2de::Config* g_theConfig;
extern a2de::UISystem* g_theUISystem;
extern a2de::InputSystem* g_theInputSystem;
extern a2de::AudioSystem* g_theAudioSystem;
extern App* g_theApp;
extern Game* g_theGame;
extern a2de::EngineSubsystem* g_theSubsystemHead;

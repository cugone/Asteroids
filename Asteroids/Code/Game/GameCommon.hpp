#pragma once

class JobSystem;
class FileLogger;
class Renderer;
class Console;
class Config;
class UISystem;
class InputSystem;
class AudioSystem;
class EngineSubsystem;

class App;
class Game;

extern JobSystem* g_theJobSystem;
extern FileLogger* g_theFileLogger;
extern Renderer* g_theRenderer;
extern Console* g_theConsole;
extern Config* g_theConfig;
extern UISystem* g_theUISystem;
extern InputSystem* g_theInputSystem;
extern AudioSystem* g_theAudioSystem;
extern App* g_theApp;
extern Game* g_theGame;
extern EngineSubsystem* g_theSubsystemHead;

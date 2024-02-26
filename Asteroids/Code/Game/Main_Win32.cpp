
#include "Engine/Core/EngineBase.hpp"

#include "Game/Game.hpp"
#include "Game/GameConfig.hpp"

#pragma warning(push)
#pragma warning(disable: 28251)

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    UNUSED(hInstance);
    UNUSED(hPrevInstance);
    UNUSED(pCmdLine);
    UNUSED(nCmdShow);

    Engine<Game>::Initialize(g_title_str);
    Engine<Game>::Run();
    Engine<Game>::Shutdown();
}

#pragma warning(pop)

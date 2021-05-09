
#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/Config.hpp"
#include "Engine/Core/Console.hpp"
#include "Engine/Core/EngineBase.hpp"
#include "Engine/Core/FileLogger.hpp"
#include "Engine/Core/KeyValueParser.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Win.hpp"

#include "Engine/Memory/MemoryPool.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Window.hpp"

#include "Game/GameCommon.hpp"
#include "Game/App.hpp"

#include <sstream>
#include <memory>
#include <vector>

#pragma warning(push)
#pragma warning(disable: 28251)

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow);
void Initialize(HINSTANCE hInstance, PWSTR pCmdLine);
void MainLoop();
void RunMessagePump();
void Shutdown();

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    UNUSED(hPrevInstance);
    UNUSED(nCmdShow);

    Initialize(hInstance, pCmdLine);
    MainLoop();
    Shutdown();
}

#pragma warning(pop)

void Initialize(HINSTANCE hInstance, PWSTR pCmdLine) {
    UNUSED(hInstance);
    auto cmdString = a2de::StringUtils::ConvertUnicodeToMultiByte(std::wstring(pCmdLine ? pCmdLine : L""));
    g_theApp = new App(cmdString);
    g_theApp->Initialize();
}

void MainLoop() {
    while(!g_theApp->IsQuitting()) {
        RunMessagePump();
        g_theApp->RunFrame();
    }
}

void RunMessagePump() {
    MSG msg{};
    for(;;) {
        if(const bool hasMsg = !!::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE); !hasMsg) {
            break;
        }
        const auto hAccelTable = static_cast<HACCEL>(g_theConsole->GetAcceleratorTable());
        const auto hWnd = static_cast<HWND>(g_theRenderer->GetOutput()->GetWindow()->GetWindowHandle());
        if(!::TranslateAccelerator(hWnd, hAccelTable, &msg)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }
}

void Shutdown() {
    delete g_theApp;
    g_theApp = nullptr;
}

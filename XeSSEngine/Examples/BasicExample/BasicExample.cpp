#include "BasicExampleApp.h"
#include "Core/Logger.h"
#include "Core/Exception.h"
#include <iostream>

using namespace BasicExample;
using namespace XeSS;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Set DPI awareness
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

    try {
        // Configure logging
        Logger::Instance().SetLevel(LogLevel::Info);

        XESS_INFO("Starting XeSS Engine Basic Example");

        // Create and run application
        BasicExampleApp app;
        int result = app.Run();

        XESS_INFO("Application finished with code: {}", result);
        return result;
    }
    catch (const std::exception& e) {
        XESS_CRITICAL("Unhandled exception: {}", e.what());

        std::string message = "XeSS Engine Error:\n" + std::string(e.what());
        MessageBoxA(nullptr, message.c_str(), "Error", MB_OK | MB_ICONERROR);

        return -1;
    }
    catch (...) {
        XESS_CRITICAL("Unknown exception occurred");

        MessageBoxA(nullptr, "An unknown error occurred", "Error", MB_OK | MB_ICONERROR);

        return -1;
    }
}

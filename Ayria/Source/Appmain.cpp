/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-04-17
    License: MIT
*/

#include <Stdinclude.hpp>
#include <Global.hpp>

namespace Backend
{
    static void setThreadname(std::string_view Name)
    {
        #pragma pack(push, 8)
        using THREADNAME_INFO = struct { DWORD dwType; LPCSTR szName; DWORD dwThreadID; DWORD dwFlags; };
        #pragma pack(pop)

        __try
        {
            THREADNAME_INFO Info{ 0x1000, Name.data(), 0xFFFFFFFF };
            RaiseException(0x406D1388, 0, sizeof(Info) / sizeof(ULONG_PTR), (ULONG_PTR *)&Info);
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static DWORD __stdcall Mainthread(void *)
    {
        // As we are single-threaded (in release), boost our priority.
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

        // Name this thread for easier debugging.
        if constexpr (Build::isDebug) setThreadname("Ayria_Main");

        // Initialize the subsystems.
        // TODO(tcn): Initialize networking.
        // TODO(tcn): Initialize pluginmenu.
        Overlay_t Ingameconsole({}, {});
        Console::Overlay::Createconsole(&Ingameconsole);

        // Optional console for developers, runs its own thread.
        if(std::strstr(GetCommandLineA(), "-DEVCON")) Console::Windows::Showconsole(false);

        // Main loop, runs until the application terminates or DLL unloads.
        std::chrono::high_resolution_clock::time_point Lastframe{};
        while (true)
        {
            // Track the frame-time, should be around 33ms.
            const auto Thisframe{ std::chrono::high_resolution_clock::now() };
            const auto Deltatime = std::chrono::duration<float>(Thisframe - Lastframe).count();
            Lastframe = Thisframe;

            // Notify the subsystems about a new frame.
            // TODO(tcn): Network frame.
            // TODO(tcn): Client frame.
            Ingameconsole.doFrame(Deltatime);

            // Log frame-average every 5 seconds.
            if constexpr (Build::isDebug)
            {
                static std::array<uint32_t, 256> Timings{};
                static float Elapsedtime{};
                static size_t Index{};

                Timings[Index % 256] = 1000000 * Deltatime;
                Elapsedtime += Deltatime;
                Index++;

                if (Elapsedtime >= 5.0f)
                {
                    const auto Sum = std::reduce(std::execution::par_unseq, Timings.begin(), Timings.end());
                    Debugprint(va("Average frametime[us]: %lu", Sum / 256));
                    Elapsedtime = 0;
                }
            }

            // Even with software-rendering, we can still hit 600 FPS, so slow down.
            std::this_thread::sleep_until(Lastframe + std::chrono::milliseconds(33));
        }

        return 0;
    }

    static void Initialize()
    {
        // TODO(tcn): Init client info.

        CreateThread(NULL, NULL, Mainthread, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
    }
}

// Some applications do not handle exceptions well.
static LONG __stdcall onUnhandledexception(PEXCEPTION_POINTERS Context)
{
    // OpenSSLs RAND_poll() causes Windows to throw if RPC services are down.
    if (Context->ExceptionRecord->ExceptionCode == RPC_S_SERVER_UNAVAILABLE)
    {
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    // DirectSound does not like it if the Audio services are down.
    if (Context->ExceptionRecord->ExceptionCode == RPC_S_UNKNOWN_IF)
    {
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    // Semi-documented way for naming Windows threads.
    if (Context->ExceptionRecord->ExceptionCode == 0x406D1388)
    {
        if (Context->ExceptionRecord->ExceptionInformation[0] == 0x1000)
        {
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }

    // Probably crash.
    return EXCEPTION_CONTINUE_SEARCH;
}

// Entrypoint when loaded as a shared library.
BOOLEAN __stdcall DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID lpvReserved)
{
    // On startup.
    if (nReason == DLL_PROCESS_ATTACH)
    {
        // Ensure that Ayrias default directories exist.
        std::filesystem::create_directories("./Ayria/Logs");
        std::filesystem::create_directories("./Ayria/Assets");
        std::filesystem::create_directories("./Ayria/Plugins");

        // Only keep a log for this session.
        Logging::Clearlog();

        // Catch any unwanted exceptions.
        SetUnhandledExceptionFilter(onUnhandledexception);

        // Opt out of further notifications.
        DisableThreadLibraryCalls(hDllHandle);

        // Initialize the backend; in-case plugins need access.
        Backend::Initialize();

        // If injected, we can't hook. So just load all plugins directly.
        if (lpvReserved == NULL) Plugins::Initialize();
        else
        {
            // We prefer TLS-hooks over EP.
            if (Plugins::InstallTLSHook()) return TRUE;

            // Fallback to EP hooking.
            if (!Plugins::InstallEPHook())
            {
                MessageBoxA(NULL, "Could not install a hook in the target application", "Fatal error", NULL);
                return FALSE;
            }
        }
    }

    return TRUE;
}

// Entrypoint when running as a hostprocess.
int main(int, char **)
{
    printf("Host startup..\n");

    // Register the window.
    WNDCLASSEXA Windowclass{};
    Windowclass.cbSize = sizeof(WNDCLASSEXA);
    Windowclass.lpfnWndProc = DefWindowProcW;
    Windowclass.lpszClassName = "Hostwindow";
    Windowclass.hInstance = GetModuleHandleA(NULL);
    Windowclass.hbrBackground = CreateSolidBrush(0xFF00FF);
    Windowclass.style = CS_BYTEALIGNWINDOW | CS_BYTEALIGNCLIENT;
    if (NULL == RegisterClassExA(&Windowclass)) return 2;

    // Create a simple window.
    const auto Windowhandle = CreateWindowExA(NULL, Windowclass.lpszClassName, "HOST", NULL, 1080, 720,
        1280, 720, NULL, NULL, Windowclass.hInstance, NULL);
    if (!Windowhandle) return 1;
    ShowWindow(Windowhandle, SW_SHOW);

    // Initialize the backend to test features.
    Backend::Initialize();

    // Poll until quit.
    MSG Message;
    while (GetMessageW(&Message, Windowhandle, NULL, NULL))
    {
        TranslateMessage(&Message);

        if (Message.message == WM_QUIT) return 3;
        else DispatchMessageW(&Message);
    }

    return 0;
}

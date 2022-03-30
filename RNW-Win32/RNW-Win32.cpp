// RNW-Win32.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "RNW-Win32.h"

#include <JSI/JsiApiContext.h>

#include <NativeModules.h>
#include "Dispatcher.h"
#include "RedBoxHandler.h"
#include "Win32ReactRootView.h"
#include "ModuleRegistration.h"
#include "PaperUIManager.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
 name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
 processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace winrt;
using namespace Microsoft::ReactNative;
using namespace std;
using namespace std::chrono;

ReactNativeHost host{ nullptr };
winrt::Microsoft::ReactNative::ReactContext g_context;
uint32_t reloadCount{ 1 };
bool g_exit{ false };

// Work around crash in DeviceInfo when running outside of XAML environment
REACT_MODULE(DeviceInfo)
struct DeviceInfo {
    REACT_CONSTANT_PROVIDER(ConstantsViaConstantsProvider)
        void ConstantsViaConstantsProvider(winrt::Microsoft::ReactNative::ReactConstantProvider& constants) noexcept {
        JSValueObject screen
        { {
            { "fontScale", 1 },
            { "scale", 1},
            { "height", 400 },
            { "width", 600},
        } };
        JSValueObject window{
            { "fontScale", 1 },
            { "scale", 1},
            { "height", 400 },
            { "width", 600},
        };
        JSValueObject dimensions 
        { {
            { "screen", std::move(screen) },
            { "window", std::move(window) },
        } };
        constants.Add(L"Dimensions", dimensions);

        //
        //constants.Writer().WritePropertyName(L"Dimensions");
        //constants.Writer().WriteObjectBegin();
        //dimensions.WriteTo(constants.Writer());
        //constants.Writer().WriteObjectEnd();
        //constants.Add(L"Dimensions", JSValue(std::move(dimensions)));
    }
};

struct HeadlessPackageProvider : winrt::implements<HeadlessPackageProvider, IReactPackageProvider> {
    void CreatePackage(winrt::Microsoft::ReactNative::IReactPackageBuilder const& packageBuilder) noexcept {

        //AddAttributedModules(packageBuilder);
        std::vector<std::wstring> turboModules = { L"UIManager", L"DeviceInfo" };
        for (auto const* reg = ModuleRegistration::Head(); reg != nullptr; reg = reg->Next()) {
            if (std::find(turboModules.begin(), turboModules.end(), reg->ModuleName()) != turboModules.end()) {
                // skip
            }
            else {
                packageBuilder.AddModule(reg->ModuleName(), reg->MakeModuleProvider());
            }
        }


        auto tm = packageBuilder.as<IReactPackageBuilderExperimental>();
        tm.AddTurboModule(L"UIManager", winrt::Microsoft::ReactNative::MakeModuleProvider<PaperUIManager>());
        tm.AddTurboModule(L"DeviceInfo", winrt::Microsoft::ReactNative::MakeModuleProvider<DeviceInfo>());
    }
};



struct Win32ReactViewInstance : winrt::implements<Win32ReactViewInstance, Microsoft::ReactNative::IReactViewInstance> {
    Win32ReactViewInstance(const Microsoft::ReactNative::ReactNativeHost& host, std::shared_ptr<Win32ReactRootView> rootview) :
        m_host(host),
        m_rootview(rootview)
    {
    }
    

    void InitRootView(Microsoft::ReactNative::IReactContext context, Microsoft::ReactNative::ReactViewOptions viewOptions) {
        m_context = context;
    }

    void UpdateRootView() {

    }

    void UninitRootView() {

    }
private:
    Microsoft::ReactNative::ReactNativeHost m_host{ nullptr };
    std::shared_ptr<Win32ReactRootView> m_rootview;
    Microsoft::ReactNative::IReactViewHost m_viewHost;
    Microsoft::ReactNative::IReactContext m_context;

};

std::shared_ptr<Win32ReactRootView> rootview;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int, _Out_ HWND&);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

constexpr std::string_view to_string(winrt::Microsoft::ReactNative::LogLevel value) {
    switch (value) {
    case winrt::Microsoft::ReactNative::LogLevel::Info: return "Info";
    case winrt::Microsoft::ReactNative::LogLevel::Warning: return "Warning";
    case winrt::Microsoft::ReactNative::LogLevel::Trace: return "Trace";
    case winrt::Microsoft::ReactNative::LogLevel::Error: return "Error";
    case winrt::Microsoft::ReactNative::LogLevel::Fatal: return "Fatal";
    default: throw std::invalid_argument("Invalid winrt::Microsoft::ReactNative::LogLevel value");
    }
}

fire_and_forget Start() {
    auto s = host.InstanceSettings();
    s.JavaScriptBundleFile(L"index");
    s.UseWebDebugger(false); // WebDebugger will not work since we are using JSI.
    s.UseFastRefresh(true);
    s.UseDeveloperSupport(false);
    s.UseDirectDebugger(true);
    s.JSIEngineOverride(JSIEngine::Hermes);

    // Hook up JS console function to output to the console - by default it goes to debug output.
    s.NativeLogger([](winrt::Microsoft::ReactNative::LogLevel level, winrt::hstring message) noexcept {
        auto prefix = "NativeLogger [" + std::string(to_string(level)) + "] ";
        OutputDebugStringA(prefix.c_str());
        OutputDebugStringW(message.c_str());
        OutputDebugStringA("\n");
    });

    auto token = s.InstanceCreated([] (
        winrt::Windows::Foundation::IInspectable const& sender, const InstanceCreatedEventArgs& args) -> winrt::fire_and_forget {
            auto context = React::ReactContext(args.Context());
            g_context = context;
            
            rootview->ReactNativeHost(host);

            rootview->Start(g_context);
            auto viewInstance = winrt::make_self<Win32ReactViewInstance>(host, rootview);
            rootview->m_viewHost.AttachViewInstance(viewInstance.as<IReactViewInstance>());
            return {};
        });
    //s.UseDirectDebugger(true);
    //s.DebuggerBreakOnNextLine(true); // Doesn't work with Chakra

    s.RedBoxHandler(winrt::make<Win32RedBoxHandler>(
        []() noexcept {++reloadCount; host.ReloadInstance(); },
        []() noexcept { host.UnloadInstance(); }));

    s.PackageProviders().Append(winrt::make<HeadlessPackageProvider>());

    s.UIDispatcher(winrt::make<MockDispatcher>(g_uiDispatcher));

    co_await host.LoadInstance();

    while (!g_exit) {
        co_await 100ms;
    }

    std::cout << "\nExiting...\n";
    co_await host.UnloadInstance();

}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_RNWWIN32, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    HWND hwnd;
    if (!InitInstance (hInstance, nCmdShow, hwnd))
    {
        return FALSE;
    }

  
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RNWWIN32));

    MSG msg;

#pragma region RNW
    winrt::init_apartment(winrt::apartment_type::multi_threaded);
    host = ReactNativeHost();

    rootview = std::make_shared<Win32ReactRootView>(hwnd);
    rootview->JSComponentName("example");

    //bool exit{ false };
    host.InstanceSettings().InstanceDestroyed([](auto&& sender, auto&& args) {
        --reloadCount;
        if (reloadCount == 0) {
            g_exit = true; // Only exit if we are not just reloading another instance
        }
        });

    Start();
#pragma endregion



    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        g_uiDispatcher.RunAll();
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RNWWIN32));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_RNWWIN32);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, _Out_ HWND& hWnd)
{
   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

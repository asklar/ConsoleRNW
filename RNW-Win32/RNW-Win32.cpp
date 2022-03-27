// RNW-Win32.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "RNW-Win32.h"

#include <JSI/JsiApiContext.h>

#include <NativeModules.h>
#include "Dispatcher.h"
#include "RedBoxHandler.h"

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
};

struct HeadlessPackageProvider : winrt::implements<HeadlessPackageProvider, IReactPackageProvider> {
    void CreatePackage(winrt::Microsoft::ReactNative::IReactPackageBuilder const& packageBuilder) noexcept {
        AddAttributedModules(packageBuilder);
    }
};



//
//struct Win32ReactViewHost : winrt::implements<Win32ReactViewHost, Microsoft::ReactNative::IReactViewHost> {
//
//};



struct Win32ReactRootView : std::enable_shared_from_this<Win32ReactRootView> {
    // facebook::react::IReactRootView {

    Win32ReactRootView(HWND hostWnd) : m_wnd(hostWnd) {
    }

    //Windows::Foundation::IAsyncAction ReloadViewInstance() {
    //    return {};
    //}

    //Windows::Foundation::IAsyncAction ReloadViewInstanceWithOptions(ReactViewOptions options) {
    //    return {};
    //}

    //Windows::Foundation::IAsyncAction UnloadViewInstance() {
    //    return {};
    //}

    void Start() {
        Microsoft::ReactNative::ReactViewOptions options;
        options.ComponentName(winrt::to_hstring(m_componentName));

        m_viewHost = Microsoft::ReactNative::ReactCoreInjection::MakeViewHost(host, options);

        //m_viewHost.AttachViewInstance(viewInstance);
    }

    void JSComponentName(std::string_view sv) { m_componentName = sv; }
    std::string JSComponentName() const noexcept {
        return m_componentName;
    }

    void ReactNativeHost(Microsoft::ReactNative::ReactNativeHost host) {
    }

    Microsoft::ReactNative::IReactViewHost m_viewHost;
    //Microsoft::ReactNative::IReactViewInstance m_reactViewInstance;

private:
    std::string m_componentName;
    int64_t m_tag{ -1 };
    HWND m_wnd{ nullptr };
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


fire_and_forget Start() {
    auto s = host.InstanceSettings();
    s.JavaScriptBundleFile(L"index");
    s.UseWebDebugger(false); // WebDebugger will not work since we are using JSI.
    s.UseFastRefresh(true);
    s.UseDeveloperSupport(false);

    // Hook up JS console function to output to the console - by default it goes to debug output.
    s.NativeLogger([](winrt::Microsoft::ReactNative::LogLevel level, winrt::hstring message) noexcept {
        std::cout << winrt::to_string(message) << std::endl;
        });

    auto token = s.InstanceCreated([] (
        winrt::Windows::Foundation::IInspectable const& sender, const InstanceCreatedEventArgs& args) -> winrt::fire_and_forget {
            auto context = React::ReactContext(args.Context());
            g_context = context;
            
            rootview->ReactNativeHost(host);

            rootview->Start();
            auto viewInstance = winrt::make_self<Win32ReactViewInstance>(host, rootview);
            rootview->m_viewHost.AttachViewInstance(viewInstance.as<IReactViewInstance>());
            
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
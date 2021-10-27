// ConsoleApplication3.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <chrono>
#include <Windows.h>
#undef GetCurrentTime
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.ReactNative.h>

#include <JSI/JsiApiContext.h>

#include <NativeModules.h>

#include <winrt/Windows.Graphics.Display.h>

using namespace winrt;
using namespace Microsoft::ReactNative;
using namespace std::chrono;

// Work around crash in DeviceInfo when running outside of XAML environment
REACT_MODULE(DeviceInfo)
struct DeviceInfo {
};

struct HeadlessPackageProvider : winrt::implements<HeadlessPackageProvider, IReactPackageProvider> {
  void CreatePackage(winrt::Microsoft::ReactNative::IReactPackageBuilder const& packageBuilder) noexcept {
    AddAttributedModules(packageBuilder);
  }
};

// React-Native-Windows will normally show errors in a XAML element.  For a console app we need to do something else with them.
struct ConsoleRedBoxHandler : winrt::implements<ConsoleRedBoxHandler, winrt::Microsoft::ReactNative::IRedBoxHandler>
{
  ConsoleRedBoxHandler(std::function<void()>&& fnReloadInstance, std::function<void()>&& fnShutdownInstance) {
    m_fnReloadInstance = std::move(fnReloadInstance);
    m_fnShutdownInstance = std::move(fnShutdownInstance);
  }

  void ShowNewError(winrt::Microsoft::ReactNative::IRedBoxErrorInfo info, winrt::Microsoft::ReactNative::RedBoxErrorType type) noexcept {
    switch (type) {
    case winrt::Microsoft::ReactNative::RedBoxErrorType::JavaScriptSoft:
      std::cerr << "JavaScript Warning: ";
      break;
    case winrt::Microsoft::ReactNative::RedBoxErrorType::JavaScriptFatal:
      std::cerr << "JavaScript Error: ";
      break;
    case winrt::Microsoft::ReactNative::RedBoxErrorType::Native:
      std::cerr << "Native Error: ";
      break;
    }
    std::cerr << winrt::to_string(info.Message()) << std::endl;

    for (auto frame : info.Callstack()) {
      std::cerr << winrt::to_string(frame.File()) << '@' << winrt::to_string(frame.Method()) << ':' << frame.Line() << ':' << frame.Column() << std::endl;
    }

    std::cerr << std::endl;

    // If it was some kind of fatal error, we should provide a way for the dev to restart the instance
    if (type != winrt::Microsoft::ReactNative::RedBoxErrorType::JavaScriptSoft) {
      char c;
      do
      {
        std::cout << "Reload Instance? [y/n] ";
        std::cin >> c;
      } while (!std::cin.fail() && c != 'y' && c != 'n');

      if (c == 'y')
        m_fnReloadInstance();
      else
        m_fnShutdownInstance();
    }
  }

  bool IsDevSupportEnabled() noexcept {
    return true;
  }

  void UpdateError(winrt::Microsoft::ReactNative::IRedBoxErrorInfo info) noexcept {
    // no-op
  }

  void DismissRedBox() noexcept {
    // no-op
  }

private:
  std::function<void()> m_fnReloadInstance;
  std::function<void()> m_fnShutdownInstance;
};

struct Console : std::enable_shared_from_this<Console>, facebook::jsi::HostObject {
  bool exit { false };
  facebook::jsi::PropNameID logName(facebook::jsi::Runtime& rt) {
    return facebook::jsi::PropNameID::forAscii(rt, "log");
  }

  facebook::jsi::Value get(facebook::jsi::Runtime& rt, const facebook::jsi::PropNameID& nameId) noexcept override {
    if (nameId.utf8(rt) == "log") {
      return facebook::jsi::Function::createFromHostFunction(rt, logName(rt), 1, 
        [](facebook::jsi::Runtime& rt,const facebook::jsi::Value& v, const facebook::jsi::Value* args, size_t count) -> facebook::jsi::Value {
          for (size_t i = 0; i < count; i++) {
            std::cout << args[i].toString(rt).utf8(rt) << std::endl;
          }
          return facebook::jsi::Value::undefined();
        });
    }
    else if (nameId.utf8(rt) == "exit") {
      return facebook::jsi::Function::createFromHostFunction(rt, facebook::jsi::PropNameID::forAscii(rt, "exit"), 0,
        [this](facebook::jsi::Runtime& rt, const facebook::jsi::Value& v, const facebook::jsi::Value* args, size_t count) -> facebook::jsi::Value {
          this->exit = true;
          return facebook::jsi::Value::undefined();
        });
    }
    return facebook::jsi::Value::undefined();
  }

  void set(facebook::jsi::Runtime& rt, const facebook::jsi::PropNameID& name, const facebook::jsi::Value& value) noexcept override { }

  std::vector<facebook::jsi::PropNameID> getPropertyNames(facebook::jsi::Runtime& rt) noexcept override {
    return facebook::jsi::PropNameID::names({ logName(rt), facebook::jsi::PropNameID::forAscii(rt, "exit")});
  }

};


struct UIDispatcher
{
  void Post(ReactDispatcherCallback cb) {
    m_tasks.push_back(cb);
  }

  void RunAll() {
    auto tasks = std::move(m_tasks);
    for (auto task : tasks)
      task();
  }

private:
  std::vector<ReactDispatcherCallback> m_tasks;
};

UIDispatcher g_uiDispatcher;

// Super simplistic dispatcher manually pumped in main loop
struct MockDispatcher : winrt::implements<MockDispatcher, IReactDispatcher> {

  MockDispatcher() {
    m_threadId = std::this_thread::get_id();
  }

  bool HasThreadAccess() { return m_threadId == std::this_thread::get_id(); }
  void Post(ReactDispatcherCallback cb) {
    g_uiDispatcher.Post(cb);
  }

private:
  std::thread::id m_threadId;
};

std::shared_ptr<Console> console;
ReactNativeHost host{ nullptr };
uint32_t reloadCount{ 1 };

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

  //s.UseDirectDebugger(true);
  //s.DebuggerBreakOnNextLine(true); // Doesn't work with Chakra

  s.RedBoxHandler(winrt::make<ConsoleRedBoxHandler>(
    []() noexcept {++reloadCount; host.ReloadInstance(); },
    []() noexcept { host.UnloadInstance(); }));

  s.PackageProviders().Append(winrt::make<HeadlessPackageProvider>());

  s.UIDispatcher(winrt::make<MockDispatcher>());

  console = std::make_shared<Console>();
  auto token = s.InstanceCreated([](
    winrt::Windows::Foundation::IInspectable const& sender, const InstanceCreatedEventArgs& args) {
      auto context = React::ReactContext(args.Context());
      ExecuteJsi(context, [](facebook::jsi::Runtime& rt) {
        auto obj = rt.global().createFromHostObject(rt, console);

		auto consoleName = facebook::jsi::PropNameID::forAscii(rt, "nativeConsole");
        rt.global().setProperty(rt, consoleName, obj);
        });

    });

  co_await host.LoadInstance();

  while (!console->exit) { 
    co_await 100ms;
  }

  std::cout << "\nExiting...\n";
  co_await host.UnloadInstance();

}

int main()
{
  winrt::init_apartment(winrt::apartment_type::multi_threaded);
  host = ReactNativeHost();
  bool exit = false;
  host.InstanceSettings().InstanceDestroyed([&exit](auto&& sender, auto&& args) {
    --reloadCount;
    if (reloadCount == 0) {
      exit = true; // Only exit if we are not just reloading another instance
    }
    });

  Start();


  while (!exit) {
    Sleep(100);
    g_uiDispatcher.RunAll();
  }
  winrt::uninit_apartment();
}

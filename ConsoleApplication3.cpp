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

#include <winrt/Windows.Graphics.Display.h>

using namespace winrt;
using namespace Microsoft::ReactNative;
using namespace std::chrono;
namespace React = Microsoft::ReactNative;

struct Console : std::enable_shared_from_this<Console>, facebook::jsi::HostObject {
  bool exit{ false };
  facebook::jsi::PropNameID logName(facebook::jsi::Runtime& rt) {
    return facebook::jsi::PropNameID::forAscii(rt, "log");
  }

  facebook::jsi::Value get(facebook::jsi::Runtime& rt, const facebook::jsi::PropNameID& nameId) noexcept override {
    if (nameId.utf8(rt) == "log") {
      return facebook::jsi::Function::createFromHostFunction(rt, logName(rt), 1, 
        [](facebook::jsi::Runtime& rt,const facebook::jsi::Value& v, const facebook::jsi::Value* args, size_t count) -> facebook::jsi::Value {
          for (size_t i = 0; i < count; i++) {
            std::cout << args[i].toString(rt).utf8(rt);
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

struct MockDispatcher : winrt::implements<MockDispatcher, IReactDispatcher> {
  bool HasThreadAccess() { return true; }
  void Post(ReactDispatcherCallback cb) {
    cb();
  }
};

namespace Mso {
  template<typename T>
  using Functor = std::function<T>;
}

// copied from RNW - do not modify
struct DeviceInfoHolder {
  DeviceInfoHolder() = default;

  static void SetCallback(
    const React::ReactPropertyBag& propertyBag,
    Mso::Functor<void(React::JSValueObject&&)>&& callback) noexcept {

  }
  static void InitDeviceInfoHolder(const React::ReactPropertyBag& propertyBag) noexcept {}
  static React::JSValueObject GetDimensions(const React::ReactPropertyBag& propertyBag) noexcept {
    return React::JSValueObject{};
  }

private:
  //React::JSValueObject getDimensions() noexcept;
  //void updateDeviceInfo() noexcept;
  //void notifyChanged() noexcept;

  float m_windowWidth{ 0 };
  float m_windowHeight{ 0 };
  float m_scale{ 0 };
  double m_textScaleFactor{ 0 };
  float m_dpi{ 0 };
  uint32_t m_screenWidth{ 0 };
  uint32_t m_screenHeight{ 0 };

  winrt::Windows::UI::Core::CoreWindow::SizeChanged_revoker m_sizeChangedRevoker;
  winrt::Windows::Graphics::Display::DisplayInformation::DpiChanged_revoker m_dpiChangedRevoker{};
  Mso::Functor<void(React::JSValueObject&&)> m_notifyCallback;
};

static const React::ReactPropertyId<React::ReactNonAbiValue<std::shared_ptr<DeviceInfoHolder>>>
& DeviceInfoHolderPropertyId() noexcept {
  static const React::ReactPropertyId<React::ReactNonAbiValue<std::shared_ptr<DeviceInfoHolder>>> prop{
      L"ReactNative.DeviceInfo", L"DeviceInfoHolder" };
  return prop;
}

std::shared_ptr<Console> console;
ReactNativeHost host{ nullptr };

fire_and_forget Start() {
  auto s = host.InstanceSettings();
  s.JavaScriptBundleFile(L"index");
  s.UseWebDebugger(false);
  s.UseFastRefresh(true);
  s.UseDeveloperSupport(false);

  s.UIDispatcher(winrt::make<MockDispatcher>());
  auto dih = std::make_shared<DeviceInfoHolder>();
  auto props = ReactPropertyBag(s.Properties());
  props.Set(DeviceInfoHolderPropertyId(), std::move(dih));

  console = std::make_shared<Console>();
  auto token = s.InstanceCreated([](
    winrt::Windows::Foundation::IInspectable const& sender, const InstanceCreatedEventArgs& args) {
      auto context = React::ReactContext(args.Context());
      ExecuteJsi(context, [](facebook::jsi::Runtime& rt) {
        auto obj = rt.global().createFromHostObject(rt, console);

        static auto consoleName = facebook::jsi::PropNameID::forAscii(rt, "nativeConsole");
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
    exit = true;
    });

  Start();
    
  while (!exit) { Sleep(100); }
  winrt::uninit_apartment();
}

// ConsoleApplication3.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.ReactNative.h>
#include <NativeModules.h>
using namespace winrt;
using namespace Microsoft::ReactNative;

namespace winrt::example::implementation
{
  struct ReactPackageProvider : winrt::implements<ReactPackageProvider, winrt::Microsoft::ReactNative::IReactPackageProvider>
  {
  public: // IReactPackageProvider
    void CreatePackage(winrt::Microsoft::ReactNative::IReactPackageBuilder const& packageBuilder) noexcept {
      AddAttributedModules(packageBuilder);
    }
  };
} // namespace winrt::example::implementation



struct ConsolePackageProvider : IReactPackageProvider {

};

fire_and_forget start() {
  ReactNativeHost host;
  auto s = host.InstanceSettings();
  s.JavaScriptBundleFile(L"index");
  s.UseWebDebugger(true);
  s.UseFastRefresh(true);
  s.UseDeveloperSupport(false);

  host.PackageProviders().Append(ConsolePackageProvider());
  auto loadOp = host.LoadInstance();
  loadOp.get();
  return{};
}

int main()
{
  start();
  
  while (true) { Sleep(1000); }
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

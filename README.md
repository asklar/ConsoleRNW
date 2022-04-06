# React Native Win32
This repro implements a proof of concept for React Native on Win32, by using most of the functionality in React Native for Windows (react-native-windows).

- "React Native Console". ConsoleApplication3 is a console mode app that uses RNW for communicating with JS. No View Managers, but uses JSI to expose native modules that can be called from JS (e.g. nativeConsole).
- "RNW-Win32" includes a GDI renderer for `<View>`

The RNW-Win32 project was built after building the console one, so everything that was required for the console endpoint applies to Win32.

## React Native Console

### What was needed to stand it up

- Override the `DeviceInfo` module - this uses XAML in RNW, so must stub it out. Since we won't have view managers, it doesn't need to do anything.
- Override the `LogBox` module, as it uses XAML in RNW. Only needs `show()` and `hide()`.
- Override the `Timing` module, as it uses `DispatcherQueue` and/or `CompositionTarget.Rendering` in RNW.
- Provide a custom RedBox handler. This gets passed into the RNW host's `InstanceSettings` via `RedBoxHandler`.
- Register a JSI host object that implements a `nativeConsole.log(...)` API (it just uses `std::cout` internally).
- Provide a custom `UIDispatcher`; it's enough to keep track of a vector of tasks to execute. 
- Provide a custom `IReactDispatcher` (MockDispather). This just talks to the UIDispatcher in a thread-affine way (keeps track of the thread it was created on).
- Override the `NativeLogger` in the RN host `InstanceSettings` (this can just call `std::cout`).
- Manually provide a ReactPackageProvider (`HeadlessPackageProvider`), add it to the host's `PackageProviders`.

On instance created, we register the native console HostObject, and more importantly, we capture the `ReactContext`.

## React Native Win32

### What was needed to stand it up

- `DeviceInfo` needs to expose its constants once we start trying to have views (screen and window dimensions, scale, font scale).
- Override the `UIManager` turbomodule ("PaperUIManager")
  It's important that both UIManager and DeviceInfo be registered as turbomodules, not as native modules, since there was a bug in RNW (which has since been fixed, but not in the version I'm using) that prevented app-provided modules from overriding framework turbomodules.
- Override the `Alert` module and associated `ShowAlertArgs`; for Win32 it will call MessageBox.
- Define a custom `IReactViewInstance` (Win32ReactViewInstance). This contains pointers to the RN host, a custom root view (Win32ReactRootView), a view host (provided by RNW as a IReactViewHost) and the React context.
- Upon instance created, we will also create and initialize the custom root view. The root view creates the view host via `ReactCoreInjection::MakeViewHost()`
  We will also call the JS function AppRegistry.runApplication and pass it the main module name, the initial props, and the root tag.
  Create a view instance, and connect it to the host and root view.
  We will then call `IReactViewHost.AttachViewInstance` on the view host that RNW provides, and pass our IReactViewInstance.
- In our message loop, we need to process window messages but we also need to process tasks posted to the UI dispatcher. We achieve both by creating a 1ms timer which will queue WM_TIMER messages, and when we receive these we will drain the UIDispatcher.

The rest of the work is mainly in PaperUIManager, and its own internal notion of what view managers are.

#### UIManager
The UI manager defines its own notion of view managers (separate from RNW's C++ ones, and from the ABI safe ones).
To run code in the UI thread, the UI manager can use `ReactCoreInjection::PostToUIBatchingQueue`.
It also defines shadow nodes, and needs to call into Yoga. For this I had to duplicate a lot of the logic in RNW's PaperUIManager and NativeUIManager.
Some of the duplicated code falls into either:
- converters between React properties (names) and Yoga enums/values (we should see if some of these Yoga helpers can be exposed through M.RN.Cxx).

Customizations:
- Keeping track of shadow nodes; these won't be XAML based, but HWND based.
- DoLayout needs to read from Yoga and use HWND APIs instead of XAML to set the component position/size.
- `setChildren` needs to manually parent/un-parent yoga nodes, and do the same for the underlying HWNDs.



#pragma once

struct Win32ReactRootView : std::enable_shared_from_this<Win32ReactRootView> {

    Win32ReactRootView(HWND hostWnd) : m_wnd(hostWnd) {
    }
    int64_t Tag() const noexcept { return m_tag; }

    HWND Window() const noexcept { return m_wnd; }

    void Start(winrt::Microsoft::ReactNative::ReactContext context);

    void JSComponentName(std::string_view sv) { m_componentName = sv; }
    std::string JSComponentName() const noexcept {
        return m_componentName;
    }

    void ReactNativeHost(winrt::Microsoft::ReactNative::ReactNativeHost host) {
    }

    winrt::Microsoft::ReactNative::IReactViewHost m_viewHost;

private:
    HWND m_wnd{ nullptr };
    std::string m_componentName;
    int64_t m_tag{ -1 };
};


#pragma once
#include <sstream>

// React-Native-Windows will normally show errors in a XAML element.  For a console app we need to do something else with them.
struct Win32RedBoxHandler : winrt::implements<Win32RedBoxHandler, winrt::Microsoft::ReactNative::IRedBoxHandler>
{
    Win32RedBoxHandler(std::function<void()>&& fnReloadInstance, std::function<void()>&& fnShutdownInstance) {
        m_fnReloadInstance = std::move(fnReloadInstance);
        m_fnShutdownInstance = std::move(fnShutdownInstance);
    }

    void ShowNewError(winrt::Microsoft::ReactNative::IRedBoxErrorInfo info, winrt::Microsoft::ReactNative::RedBoxErrorType type) noexcept {
        auto msg = winrt::to_string(info.Message());
        std::string title;
        switch (type) {
        case winrt::Microsoft::ReactNative::RedBoxErrorType::JavaScriptSoft:
            title = "JavaScript Warning: ";
            break;
        case winrt::Microsoft::ReactNative::RedBoxErrorType::JavaScriptFatal:
            title = "JavaScript Error: ";
            break;
        case winrt::Microsoft::ReactNative::RedBoxErrorType::Native:
            title = "Native Error: ";
            break;
        }

        std::stringstream ss;
        ss << msg << std::endl;

        for (auto frame : info.Callstack()) {
            ss << winrt::to_string(frame.File()) << '@' << winrt::to_string(frame.Method()) << ':' << frame.Line() << ':' << frame.Column() << std::endl;
        }

        ss << std::endl;

        auto result = MessageBoxA(nullptr, ss.str().c_str(), title.c_str(), MB_RETRYCANCEL);
        // If it was some kind of fatal error, we should provide a way for the dev to restart the instance
        if (type != winrt::Microsoft::ReactNative::RedBoxErrorType::JavaScriptSoft) {
            if (result == IDRETRY)
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


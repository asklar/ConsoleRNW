#pragma once
#include "IWin32ViewManager.h"

#include <../../.fmt/fmt-7.1.3/include/fmt/format.h>

struct ViewViewManager : IWin32ViewManager {

    enum class ClassIndex {
        Background = 0,
        Last,
    };

    ViewViewManager();

    static LRESULT __stdcall ViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    ~ViewViewManager();

    HWND Create(int64_t reactTag, int64_t rootTag, HWND rootHWnd, const winrt::Microsoft::ReactNative::JSValueObject& props);

    void UpdateProperties(int64_t reactTag, HWND hwnd, const winrt::Microsoft::ReactNative::JSValueObject& props) override;

    winrt::Microsoft::ReactNative::JSValueObject GetConstants();
};


#pragma once
#include <windows.h>
#include "ShadowNode.h"
#include <JSValue.h>

struct IWin32ViewManager {
    virtual ~IWin32ViewManager() = 0 {};
    virtual HWND Create(int64_t reactTag, int64_t rootTag, HWND rootHWnd, const winrt::Microsoft::ReactNative::JSValueObject& props) = 0;
    virtual winrt::Microsoft::ReactNative::JSValueObject GetConstants() = 0;
    virtual void UpdateProperties(int64_t reactTag, HWND hwnd, const winrt::Microsoft::ReactNative::JSValueObject& props) = 0;

    static ShadowNode* GetShadowNode(HWND hwnd) {
        return reinterpret_cast<ShadowNode*>(GetWindowLongPtrW(hwnd, 0));
    }
    static void SetShadowNode(HWND hwnd, ShadowNode* node) {
        SetWindowLongPtrW(hwnd, 0, reinterpret_cast<LONG_PTR>(node));
    }
};

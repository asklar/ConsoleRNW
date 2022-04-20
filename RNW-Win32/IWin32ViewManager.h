#pragma once
#include <windows.h>
#include "ShadowNode.h"
#include <JSValue.h>

struct ShadowNode;
struct PaperUIManager;

struct IWin32ViewManager {
    virtual ~IWin32ViewManager() = 0 {};
    virtual std::shared_ptr<ShadowNode> Create(int64_t reactTag, int64_t rootTag, HWND rootHWnd, const winrt::Microsoft::ReactNative::JSValueObject& props) = 0;
    virtual winrt::Microsoft::ReactNative::JSValueObject GetConstants() = 0;
    virtual void UpdateProperties(int64_t reactTag, std::shared_ptr<ShadowNode> node, const winrt::Microsoft::ReactNative::JSValueObject& props) = 0;

    virtual void UpdateLayout(ShadowNode* node, int left, int top, int width, int height) = 0;
    static ShadowNode* GetShadowNode(HWND hwnd) {
        return reinterpret_cast<ShadowNode*>(GetWindowLongPtrW(hwnd, 0));
    }
    static void SetShadowNode(HWND hwnd, ShadowNode* node) {
        SetWindowLongPtrW(hwnd, 0, reinterpret_cast<LONG_PTR>(node));
    }
    static int64_t GetTag(HWND hwnd) {
        return reinterpret_cast<int64_t>(GetProp(hwnd, L"Tag"));
    }
    static void SetTag(HWND hwnd, int64_t tag) {
        SetProp(hwnd, L"Tag", reinterpret_cast<HANDLE>(tag));
    }


    IWin32ViewManager(winrt::Microsoft::ReactNative::ReactContext ctx, YGConfigRef config) : m_context(ctx), m_yogaConfig(config) {}

    template<typename T>
    auto EmitEvent(std::string_view evtName, uint64_t tag, T&& args) {
        return m_context.CallJSFunction(
            L"RCTEventEmitter",
            L"receiveEvent",
            [tag, evtName, args = std::move(args)](const winrt::Microsoft::ReactNative::IJSValueWriter& paramsWriter) mutable {
                paramsWriter.WriteArrayBegin();
                WriteValue(paramsWriter, tag);
                WriteValue(paramsWriter, evtName);
                WriteValue(paramsWriter, args);
                paramsWriter.WriteArrayEnd();
            });
    }

    virtual YGMeasureFunc GetCustomMeasureFunction() {
        return nullptr;
    }

    winrt::Microsoft::ReactNative::ReactContext m_context;
    YGConfigRef m_yogaConfig;

    std::shared_ptr<PaperUIManager> GetUIManager() const;
};

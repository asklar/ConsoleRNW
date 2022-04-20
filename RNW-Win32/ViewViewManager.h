#pragma once
#include "IWin32ViewManager.h"

#include <../../.fmt/fmt-7.1.3/include/fmt/format.h>

enum class ViewKind {
    View = 0,
    RawText = 1,
    Text = 2,
    CommonControl = 3,
};

struct ViewViewManager : IWin32ViewManager {

    ViewViewManager(winrt::Microsoft::ReactNative::ReactContext ctx, ViewKind kind, YGConfigRef yogaConfig);

    static LRESULT __stdcall ViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    ~ViewViewManager();

    std::shared_ptr<ShadowNode> Create(int64_t reactTag, int64_t rootTag, HWND rootHWnd, const winrt::Microsoft::ReactNative::JSValueObject& props);

    void UpdateProperties(int64_t reactTag, std::shared_ptr<ShadowNode> node, const winrt::Microsoft::ReactNative::JSValueObject& props) override;

    winrt::Microsoft::ReactNative::JSValueObject GetConstants() override;

    void UpdateLayout(ShadowNode* node, int left, int top, int width, int height) override;
private:
    ViewKind m_kind;
    const wchar_t* GetWindowClassName() const;
};

struct TextViewManager : ViewViewManager {
    TextViewManager(winrt::Microsoft::ReactNative::ReactContext ctx, YGConfigRef ycr) : ViewViewManager(ctx, ViewKind::Text, ycr) {}
    YGMeasureFunc GetCustomMeasureFunction() override;
    std::shared_ptr<ShadowNode> Create(int64_t reactTag, int64_t rootTag, HWND rootHWnd, const winrt::Microsoft::ReactNative::JSValueObject& props);
    void UpdateProperties(int64_t reactTag, std::shared_ptr<ShadowNode> node, const winrt::Microsoft::ReactNative::JSValueObject& props) override;
//    winrt::Microsoft::ReactNative::JSValueObject GetConstants() override;
    void UpdateLayout(ShadowNode* node, int left, int top, int width, int height) override;
};

struct RawTextViewManager : IWin32ViewManager {
    RawTextViewManager(winrt::Microsoft::ReactNative::ReactContext ctx, YGConfigRef ycr) : IWin32ViewManager(ctx, ycr) {}
    std::shared_ptr<ShadowNode> Create(int64_t reactTag, int64_t rootTag, HWND rootHWnd, const winrt::Microsoft::ReactNative::JSValueObject& props);
    void UpdateProperties(int64_t reactTag, std::shared_ptr<ShadowNode> node, const winrt::Microsoft::ReactNative::JSValueObject& props) override;
    winrt::Microsoft::ReactNative::JSValueObject GetConstants() override;
    void UpdateLayout(ShadowNode* node, int left, int top, int width, int height) override;
    YGMeasureFunc GetCustomMeasureFunction() override;
};

struct ButtonViewManager : ViewViewManager {
    ButtonViewManager(winrt::Microsoft::ReactNative::ReactContext ctx, YGConfigRef ycr) : ViewViewManager(ctx, ViewKind::CommonControl, ycr) {}
    std::shared_ptr<ShadowNode> Create(int64_t reactTag, int64_t rootTag, HWND rootHWnd, const winrt::Microsoft::ReactNative::JSValueObject& props);
    void UpdateProperties(int64_t reactTag, std::shared_ptr<ShadowNode> node, const winrt::Microsoft::ReactNative::JSValueObject& props) override;
    winrt::Microsoft::ReactNative::JSValueObject GetConstants() override;

    //void UpdateLayout(ShadowNode* node, int left, int top, int width, int height) override;
};

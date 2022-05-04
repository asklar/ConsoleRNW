#pragma once
#include "ViewViewManager.h"


struct ScrollViewShadowNode : ShadowNode
{
    ScrollViewShadowNode(HWND w, YGConfigRef config, IWin32ViewManager* vm) :  ShadowNode(w,config,vm) {
    }

    void UpdateLayout(float left, float top, float width, float height) override;

    static constexpr const wchar_t* WindowClassName = L"RCTScrollView";
    //static constexpr bool IsCustomMeasure = true;
    //static constexpr DWORD CreationStyle = ShadowNode::CreationStyle | WS_VSCROLL | WS_BORDER;
    //YGSize Measure(float width,
    //    YGMeasureMode widthMode,
    //    float height,
    //    YGMeasureMode heightMode) override;
    LRESULT WndProc(UINT msg, WPARAM, LPARAM);
    //void PaintForeground(HDC dc) override;
    //YGSize Measure(float width,
    //    YGMeasureMode widthMode,
    //    float height,
    //    YGMeasureMode heightMode) override;

};

struct ScrollContentViewShadowNode : ScrollViewShadowNode
{
    ScrollContentViewShadowNode(HWND w, YGConfigRef config, IWin32ViewManager* vm) : ScrollViewShadowNode(w, config, vm)
    {}
    static constexpr bool IsCustomMeasure = true;
    static constexpr DWORD CreationStyle = ShadowNode::CreationStyle | WS_VSCROLL | WS_BORDER;

    YGSize Measure(float width,
    YGMeasureMode widthMode,
    float height,
    YGMeasureMode heightMode) override;
    //LRESULT WndProc(UINT msg, WPARAM, LPARAM);

};

struct ScrollContentViewManager : ViewViewManager<ScrollContentViewShadowNode> {
    ScrollContentViewManager(winrt::Microsoft::ReactNative::ReactContext context, YGConfigRef ycr) : ViewViewManager(context, ViewKind::ScrollView, ycr) {}
};

struct ScrollViewManager :
    ViewViewManager<ScrollViewShadowNode>
{
    static constexpr const wchar_t* WindowClassName = L"RCTScrollContentView";
    static constexpr bool IsCustomMeasure = true;
    static constexpr DWORD CreationStyle = ShadowNode::CreationStyle | WS_VSCROLL | WS_BORDER;

    ScrollViewManager(winrt::Microsoft::ReactNative::ReactContext context, YGConfigRef ycr) : ViewViewManager(context, ViewKind::ScrollView, ycr) {}
};


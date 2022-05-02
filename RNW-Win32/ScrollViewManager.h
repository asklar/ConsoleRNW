#pragma once
#include "ViewViewManager.h"

struct ScrollViewShadowNode : ShadowNode
{
    ScrollViewShadowNode(HWND w, YGConfigRef config, IWin32ViewManager* vm) :  ShadowNode(w,config,vm) {
    }

    static constexpr const wchar_t* WindowClassName = L"RCTScrollView";
//    static constexpr bool IsCustomMeasure = true;
    static constexpr DWORD CreationStyle = ShadowNode::CreationStyle | WS_VSCROLL;

    LRESULT WndProc(UINT msg, WPARAM, LPARAM);

    //void PaintForeground(HDC dc) override;
    //YGSize Measure(float width,
    //    YGMeasureMode widthMode,
    //    float height,
    //    YGMeasureMode heightMode) override;

};

struct ScrollViewManager :
    ViewViewManager<ScrollViewShadowNode>
{
    ScrollViewManager(winrt::Microsoft::ReactNative::ReactContext context, YGConfigRef ycr) : ViewViewManager(context, ViewKind::ScrollView, ycr) {}
};


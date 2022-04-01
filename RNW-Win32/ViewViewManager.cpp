#include "pch.h"
#include "ViewViewManager.h"
#include "IWin32ViewManager.h"

ViewViewManager::ViewViewManager(winrt::Microsoft::ReactNative::ReactContext ctx) : IWin32ViewManager(ctx) {
    WNDCLASSEX wcex{};
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.cbWndExtra = sizeof(void*);
    wcex.hInstance = nullptr;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = L"RCTView";

    wcex.lpfnWndProc = ViewWndProc;
    RegisterClassEx(&wcex);
}

static auto MouseEventArgs(int64_t tag, WPARAM wParam, LPARAM lParam) {
    auto [xPos, yPos] = MAKEPOINTS(lParam);
    return winrt::Microsoft::ReactNative::JSValueObject{
                        {"target", tag},
                        {"identifier", 0 /*pointer.identifier*/},
                        //{"pageX", pointer.positionRoot.X},
                        //{"pageY", pointer.positionRoot.Y},
                        {"locationX", xPos},
                        {"locationY", yPos},
                        //{"timestamp", pointer.timestamp},
                        //{ "pointerType", GetPointerDeviceTypeName(pointer.deviceType) },
                        //{"force", pointer.pressure},
                        {"isLeftButton", (wParam & MK_LBUTTON) != 0},
                        {"isRightButton", (wParam & MK_RBUTTON) != 0},
                        {"isMiddleButton", (wParam & MK_MBUTTON) != 0},
                        //{"isBarrelButtonPressed", pointer.isBarrelButton},
                        //{"isHorizontalScrollWheel", pointer.isHorizontalScrollWheel},
                        //{"isEraser", pointer.isEraser},
                        {"shiftKey", (wParam & MK_SHIFT) != 0 },
                        //{"ctrlKey", pointer.ctrlKey},
                        //{"altKey", pointer.altKey} };
    };
}
LRESULT __stdcall ViewViewManager::ViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto node = GetShadowNode(hwnd);
    auto tag = IWin32ViewManager::GetTag(hwnd);
    switch (msg) {
    case WM_MOUSEMOVE: {
        if (!node->m_isMouseOver && node->GetValueOrDefault<ShadowNode::OnMouseEnterProperty>()) {
            node->m_isMouseOver = true;
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd };
            TrackMouseEvent(&tme);

            node->m_vm->EmitEvent("topMouseEnter", tag, MouseEventArgs(tag, wParam, lParam));

        }
        return 0;
        break;
    }
    case WM_MOUSELEAVE: {
        node->m_isMouseOver = false;
        node->m_vm->EmitEvent("topMouseLeave", tag, MouseEventArgs(tag, wParam, lParam));
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        auto dc = BeginPaint(hwnd, &ps);
        RECT rc{};
        GetClientRect(hwnd, &rc);
        auto bkColor = node->GetValue<ShadowNode::BackgroundProperty>();

        if (bkColor.has_value()) {
            SetDCBrushColor(dc, bkColor.value());
            auto borderRadius = node->GetValueOrDefault<ShadowNode::BorderRadiusProperty>();
            auto rgn = CreateRoundRectRgn(rc.left, rc.top, rc.right, rc.bottom, borderRadius, borderRadius);
            FillRgn(dc, rgn, (HBRUSH)GetStockObject(DC_BRUSH));
            DeleteObject(rgn);
        }

        wchar_t str[100]{};
        GetWindowText(hwnd, str, ARRAYSIZE(str));
        auto bkOld = GetBkColor(dc);
        {
            SetBkColor(dc, bkColor.has_value() ? bkColor.value() : GetSysColor(COLOR_WINDOW));
            DrawText(dc, str, wcslen(str), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SetBkColor(dc, bkOld);
        }
        EndPaint(hwnd, &ps);
        return 0;
        break;
    }
    case WM_ERASEBKGND:
        return 0; // handle background in WM_PAINT
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

ViewViewManager::~ViewViewManager() {
    UnregisterClassW(L"RCTView", nullptr);
}

HWND ViewViewManager::Create(int64_t reactTag, int64_t rootTag, HWND rootHWnd, const winrt::Microsoft::ReactNative::JSValueObject& props) {
    auto tag = fmt::format(L"This is a View! its react tag is: {}", reactTag);
    auto hwnd = CreateWindow(L"RCTView", tag.c_str(), WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0,
        rootHWnd, nullptr, nullptr, nullptr);
    return hwnd;
}

struct ViewProperties {
    template<typename TProperty>
    static void Set(ShadowNode* node, const winrt::Microsoft::ReactNative::JSValue& value) {
        if constexpr (std::is_same_v<typename TProperty::type, COLORREF>) {
            auto rgb = value.AsUInt32() & 0x00ffffff; //ignore alpha
            auto colorRef = RGB(rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff);

            node->SetValue<TProperty>(colorRef);
        }
        else if constexpr (std::is_same_v<typename TProperty::type, int>) {
            node->SetValue<TProperty>(value.AsInt32());
        }
        else if constexpr (std::is_same_v<typename TProperty::type, float>) {
            node->SetValue<TProperty>(value.AsFloat());
        }
        else if constexpr (std::is_same_v<typename TProperty::type, bool>) {
            node->SetValue<TProperty>(!value.IsNull() && value.AsBoolean());
        }
        else {
            static_assert(sizeof(TProperty) == 0, "can't convert jsvalue type to property type");
        }
    }

    using setter_t = void(*)(ShadowNode*, const winrt::Microsoft::ReactNative::JSValue&);
    static inline std::unordered_map<std::string, setter_t> m_setters = { {
        { "backgroundColor", Set<ShadowNode::BackgroundProperty> },
        { "borderRadius", Set<ShadowNode::BorderRadiusProperty> },
        { "onMouseEnter", Set<ShadowNode::OnMouseEnterProperty> },
        { "onMouseLeave", Set<ShadowNode::OnMouseLeaveProperty> },
    } };

    static setter_t GetProperty(const std::string& sv) {
        if (m_setters.count(sv) != 0) {
            return m_setters.at(sv);
        }
        return nullptr;
    }
};

void ViewViewManager::UpdateProperties(int64_t reactTag, HWND hwnd, const winrt::Microsoft::ReactNative::JSValueObject& props) {
    auto node = GetShadowNode(hwnd);
    for (const auto& v : props) {
        const auto& propName = v.first;
        const auto& value = v.second;

        if (auto setter = ViewProperties::GetProperty(propName)) {
            (*setter)(node, value);
        }
        else {
            //DebugBreak();
        }
    }
}

winrt::Microsoft::ReactNative::JSValueObject ViewViewManager::GetConstants() {
    using namespace winrt::Microsoft::ReactNative;
    JSValueObject view {
        {
            { "Constants", JSValueObject{} },
        { "Commands", JSValueObject{} },
        { "NativeProps", JSValueObject{
            { "onLayout", "function" },
            { "pointerEvents", "string" },
            { "onClick", "function" },
            { "onMouseEnter", "function" },
            { "onMouseLeave", "function" },
            { "focusable", "boolean" },
            { "enableFocusRing", "boolean" },
            { "tabIndex", "number" },
            { "test123", "number"},
    } },
        { "bubblingEventTypes", JSValueObject{
    } },
        { "directEventTypes", JSValueObject{
            { "topMouseEnter", JSValueObject {
                { "registrationName", "onMouseEnter" },
            }},
            { "topMouseLeave", JSValueObject {
                { "registrationName", "onMouseLeave" },
            }},
        } },
        }
    };
    return view;
}

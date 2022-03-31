#include "pch.h"
#include "ViewViewManager.h"

ViewViewManager::ViewViewManager() {
    WNDCLASSEX wcex{};
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.cbWndExtra = sizeof(void*);
    wcex.hInstance = nullptr;
    wcex.lpszClassName = L"RCTView";

    wcex.lpfnWndProc = ViewWndProc;
    RegisterClassEx(&wcex);
}

LRESULT __stdcall ViewViewManager::ViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto node = GetShadowNode(hwnd);
    switch (msg) {
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
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
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
        else {
            static_assert(sizeof(TProperty) == 0, "can't convert jsvalue type to property type");
        }
    }

    using setter_t = void(*)(ShadowNode*, const winrt::Microsoft::ReactNative::JSValue&);
    static inline std::unordered_map<std::string, setter_t> m_setters = { {
        { "backgroundColor", Set<ShadowNode::BackgroundProperty> },
        { "borderRadius", Set<ShadowNode::BorderRadiusProperty> },
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
    auto view = JSValueObject{
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
        //{ "background", "Color"},
    } },
        { "bubblingEventTypes", JSValueObject{
    } },
        { "directEventTypes", JSValueObject{} },
        }
    };
    return view;
}

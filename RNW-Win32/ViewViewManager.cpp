#include "pch.h"
#include "ViewViewManager.h"
#include "IWin32ViewManager.h"

const wchar_t* ViewViewManager::GetWindowClassName() const {
	switch (m_kind) {
	case ViewKind::View:
		return L"RCTView";
	case ViewKind::RawText:
		return L"RCTRawText";
	case ViewKind::Text:
		return L"RCTText";
	default:
		return nullptr;
	}
}

ViewViewManager::ViewViewManager(winrt::Microsoft::ReactNative::ReactContext ctx, ViewKind kind) : IWin32ViewManager(ctx), m_kind(kind) {
	WNDCLASSEX wcex{};
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.cbWndExtra = sizeof(void*);
	wcex.hInstance = nullptr;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.lpszClassName = GetWindowClassName();
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

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

void ViewViewManager::UpdateLayout(ShadowNode* node, int left, int top, int width, int height) {
	SetWindowPos(node->window, nullptr, left, top, width, height, SWP_NOZORDER);
	if (m_kind == ViewKind::Text) {
		for (const auto& child_weak : node->m_children) {
			if (auto child = child_weak.lock()) {
				child->m_vm->UpdateLayout(child.get(), left, top, width, height);
			}
		}
	}
}

LRESULT ViewViewManager::OnPaint(ShadowNode* node) {
	PAINTSTRUCT ps{};
	auto hwnd = node->window;
	auto dc = BeginPaint(hwnd, &ps);

	RECT rc{};
	GetClientRect(hwnd, &rc);

	FrameRect(dc, &rc, (HBRUSH)(GetStockObject(BLACK_BRUSH)));

	auto bkColor = node->GetValue<ShadowNode::BackgroundProperty>();

	if (bkColor.has_value()) {
		auto old = SetDCBrushColor(dc, bkColor.value());
		auto borderRadius = node->GetValueOrDefault<ShadowNode::BorderRadiusProperty>();
		auto rgn = CreateRoundRectRgn(rc.left, rc.top, rc.right, rc.bottom, borderRadius, borderRadius);
		FillRgn(dc, rgn, (HBRUSH)GetStockObject(DC_BRUSH));
		DeleteObject(rgn);
		SetDCBrushColor(dc, old);
	}

	switch (m_kind)
	{
	case ViewKind::View: {
		wchar_t str[100]{};
		GetWindowText(hwnd, str, ARRAYSIZE(str));
		auto bkOld = GetBkColor(dc);
		{
			SetBkColor(dc, bkColor.has_value() ? bkColor.value() : GetSysColor(COLOR_WINDOW));
			DrawText(dc, str, wcslen(str), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			SetBkColor(dc, bkOld);
		}
		break;
	}
	case ViewKind::Text:
		std::for_each(node->m_children.begin(), node->m_children.end(), [](const auto& child_weak) {
			if (auto child = child_weak.lock()) {
				child->m_vm->OnPaint(child.get());
			}
		});
		break;
	case ViewKind::RawText: {
		auto text = node->GetValue<ShadowNode::TextProperty>();
		auto bkOld = GetBkColor(dc);
		{
			SetBkColor(dc, bkColor.has_value() ? bkColor.value() : GetSysColor(COLOR_WINDOW));
			DrawText(dc, text.value().c_str(), -1, &rc, 0);
			SetBkColor(dc, bkOld);
		}
		break;
	}
	default:
		break;
	}

	EndPaint(hwnd, &ps);
	return 0;
}

LRESULT __stdcall ViewViewManager::ViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	auto node = GetShadowNode(hwnd);
	auto tag = IWin32ViewManager::GetTag(hwnd);
	auto vm = node ? node->m_vm : nullptr;


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
		return node->m_vm->OnPaint(node);
		break;
	}
	case WM_ERASEBKGND:
		PAINTSTRUCT ps{};
		auto dc = BeginPaint(hwnd, &ps);
		RECT rc{};
		GetClientRect(hwnd, &rc);

		if (auto parent = node->GetParent().lock()) {
			auto bkColor = parent->GetValue<ShadowNode::BackgroundProperty>();
			if (bkColor.has_value()) {
				auto oldDCBrushColor = SetDCBrushColor(dc, bkColor.value());
				FillRect(dc, &rc, (HBRUSH)GetStockObject(DC_BRUSH));
				SetDCBrushColor(dc, oldDCBrushColor);
			}
			else {
				FillRect(dc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
			}
		}
		else {
			// the parent is gone...
		}
		EndPaint(hwnd, &ps);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

ViewViewManager::~ViewViewManager() {
	UnregisterClassW(GetWindowClassName(), nullptr);
}

HWND ViewViewManager::Create(int64_t reactTag, int64_t rootTag, HWND rootHWnd, const winrt::Microsoft::ReactNative::JSValueObject& props) {
	auto tag = fmt::format(L"This is a View! its react tag is: {}", reactTag);
	auto hwnd = CreateWindow(GetWindowClassName(), tag.c_str(), WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		rootHWnd, nullptr, nullptr, nullptr);

	OutputDebugStringW(fmt::format(L"Create {} {}\n", reactTag, GetWindowClassName()).c_str());

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
		else if constexpr (std::is_same_v<typename TProperty::type, std::wstring_view>) {
			auto hstr = winrt::to_hstring(value.AsString());
			node->SetValue<TProperty>(hstr.c_str());
		}
		else {
			static_assert(sizeof(TProperty) == 0, "can't convert jsvalue type to property type");
		}
	}

	using setter_t = void(*)(ShadowNode*, const winrt::Microsoft::ReactNative::JSValue&);
	constexpr static struct {
		const char* const propertyName;
		setter_t setter;
	} m_setters[] = {
		{ "backgroundColor", Set<ShadowNode::BackgroundProperty> },
		{ "borderRadius", Set<ShadowNode::BorderRadiusProperty> },
		{ "onMouseEnter", Set<ShadowNode::OnMouseEnterProperty> },
		{ "onMouseLeave", Set<ShadowNode::OnMouseLeaveProperty> },
		{ "text", Set<ShadowNode::TextProperty> },
	};

	static setter_t GetProperty(const std::string& sv) {
		if (auto found = std::find_if(m_setters, m_setters + std::size(m_setters), 
			[sv](const auto& e) { return e.propertyName == sv; })) {
			return found->setter;
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
	JSValueObject view;
	if (m_kind == ViewKind::View) {
		view = {
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
					//{ "test123", "number"},
				} },
				{ "bubblingEventTypes", JSValueObject{} },
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
	}
	else if (m_kind == ViewKind::RawText) {
		view = {
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
					{ "text", "string" },
				} },
				{ "bubblingEventTypes", JSValueObject{} },
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
	}

	return view;
}

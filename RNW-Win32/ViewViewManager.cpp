#include "pch.h"
#include "ViewViewManager.h"
#include "IWin32ViewManager.h"
#include "PaperUIManager.h"

using JSValueObject = winrt::Microsoft::ReactNative::JSValueObject;

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



ViewViewManager::ViewViewManager(winrt::Microsoft::ReactNative::ReactContext ctx, ViewKind kind, YGConfigRef yogaConfig) : IWin32ViewManager(ctx, yogaConfig), m_kind(kind) {
	if (auto clsName = GetWindowClassName()) {
		WNDCLASSEX wcex{};
		wcex.cbSize = sizeof(wcex);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.cbWndExtra = sizeof(void*);
		wcex.hInstance = nullptr;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.lpszClassName = clsName;
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

		wcex.lpfnWndProc = ViewWndProc;
		RegisterClassEx(&wcex);
	}
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


LRESULT __stdcall ViewViewManager::ViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	auto node = GetShadowNode(hwnd);
	auto tag = IWin32ViewManager::GetTag(hwnd);
	auto vm = node ? node->m_vm : nullptr;


	switch (msg) {
	case WM_NCHITTEST: {
		if (!node->GetValueOrDefault<ShadowNode::IsMouseOverProperty>() && (!node->WantsMouseMove() || !node->GetValueOrDefault<ShadowNode::OnMouseEnterProperty>())) {
			return HTTRANSPARENT;
		}
		return HTCLIENT;
		break;
	}
	case WM_LBUTTONDOWN: {
		if (node->GetValueOrParent<ShadowNode::OnPressProperty>()) {
			node->m_vm->EmitEvent("topClick", tag, MouseEventArgs(tag, wParam, lParam));
		}
		return 0;
	}
	case WM_MOUSEMOVE: {
		//vm->GetUIManager()->PrintNodes();
		
		if (!node->WantsMouseMove()) {
			__noop;
		}
		else {
			if (!node->GetValueOrDefault<ShadowNode::IsMouseOverProperty>() && node->GetValueOrDefault<ShadowNode::OnMouseEnterProperty>()) {
				node->SetValue<ShadowNode::IsMouseOverProperty>(true);
				OutputDebugStringA(fmt::format("MouseMove tag={} vmKind={}\n", tag, typeid(*node->m_vm).name()).c_str());

				TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd };
				TrackMouseEvent(&tme);

				node->m_vm->EmitEvent("topMouseEnter", tag, MouseEventArgs(tag, wParam, lParam));

			}
			return 0;
		}
		break;
	}
	case WM_MOUSELEAVE: {
		node->SetValue<ShadowNode::IsMouseOverProperty>(false);
		OutputDebugStringA(fmt::format("MouseLeave tag={} vmKind={}\n", tag, typeid(*node->m_vm).name()).c_str());
		node->m_vm->EmitEvent("topMouseLeave", tag, MouseEventArgs(tag, wParam, lParam));
		break;
	}
	case WM_PAINT: {
		return node->OnPaint();
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

std::shared_ptr<ShadowNode> ViewViewManager::Create(int64_t reactTag, int64_t rootTag, HWND rootHWnd, const winrt::Microsoft::ReactNative::JSValueObject& props) {
	auto tag = fmt::format(L"{}: {}", GetWindowClassName(), reactTag);
	auto hwnd = CreateWindow(GetWindowClassName(), tag.c_str(), WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		rootHWnd, nullptr, nullptr, nullptr);

	OutputDebugStringW(fmt::format(L"Create {} {}\n", reactTag, GetWindowClassName()).c_str());

	IWin32ViewManager::SetTag(hwnd, reactTag);
	switch (m_kind)
	{
	case ViewKind::View:
		return std::make_shared<ShadowNode>(hwnd, m_yogaConfig, this);
		break;
	//case ViewKind::RawText:
	//	return std::make_shared<RawTextShadowNode>(hwnd, m_yogaConfig, this); 
	//	break;
	case ViewKind::Text:
		return std::make_shared<TextShadowNode>(hwnd, m_yogaConfig, this);
		break;
	default:
		throw std::invalid_argument("View Manager kind");
		break;
	}
	
}




void ViewViewManager::UpdateProperties(int64_t reactTag, std::shared_ptr<ShadowNode> node, const winrt::Microsoft::ReactNative::JSValueObject& props) {
	for (const auto& v : props) {
		const auto& propName = v.first;
		const auto& value = v.second;

		OutputDebugStringA(fmt::format("UpdateProperties: {} {}\n", reactTag, propName).c_str());
		if (auto setter = ViewProperties::GetProperty(propName)) {
			setter->setter(node.get(), value);
			if (setter->dirtyLayout) {
				GetUIManager()->DirtyYogaNode(reactTag);
			}
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
					{ "onPress", "function" },
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
					{ "topClick", JSValueObject {
						{ "registrationName", "onClick" },
					}},
				} },
			}
		};
	}

	return view;
}


LRESULT ShadowNode::OnPaint(HDC dc) {
	PAINTSTRUCT ps{};
	if (!dc) {
		dc = BeginPaint(window, &ps);
	}
	auto dbg_tag = IWin32ViewManager::GetTag(window);
	PaintBackground(dc);
	PaintForeground(dc);
	EndPaint(window, &ps);
	return 0;
}

void ShadowNode::PaintBackground(HDC dc) {
	auto hwnd = window;

	RECT rc{};
	GetClientRect(hwnd, &rc);


	auto bkColor = GetValue<ShadowNode::BackgroundProperty>();

	auto borderRadius = GetValueOrDefault<ShadowNode::BorderRadiusProperty>();
	auto rgn = CreateRoundRectRgn(rc.left, rc.top, rc.right, rc.bottom, borderRadius, borderRadius);

	if (bkColor.has_value()) {
		auto old = SetDCBrushColor(dc, bkColor.value());
		FillRgn(dc, rgn, (HBRUSH)GetStockObject(DC_BRUSH));
		SetDCBrushColor(dc, old);
	}

	auto borderColor = GetValue<ShadowNode::BorderColorProperty>();
	if (borderColor.has_value()) {
		auto brush = CreateSolidBrush(borderColor.value());
		auto bwidth = GetValueOrDefault<ShadowNode::BorderWidthProperty>();
		FrameRgn(dc, rgn, brush, bwidth, bwidth);
		DeleteObject(brush);
	}

	DeleteObject(rgn);

}

void ShadowNode::PaintForeground(HDC dc) {
	wchar_t str[100]{};
	GetWindowText(window, str, ARRAYSIZE(str));
	auto bkColor = GetValue<ShadowNode::BackgroundProperty>();
	RECT rc{};
	GetClientRect(window, &rc);
	static bool dbgShowReactTag = false;
	if (dbgShowReactTag) {
		auto bkOld = GetBkColor(dc);
		{
			SetBkColor(dc, bkColor.has_value() ? bkColor.value() : GetSysColor(COLOR_WINDOW));
			DrawText(dc, str, static_cast<int>(wcslen(str)), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			SetBkColor(dc, bkOld);
			FrameRect(dc, &rc, (HBRUSH)GetStockObject(DC_BRUSH));
		}
	}
}

void TextShadowNode::PaintForeground(HDC dc) {
	std::for_each(m_children.begin(), m_children.end(), [dc](const auto& child_weak) {
		if (auto child = child_weak.lock()) {
			child->OnPaint(dc);
		}
		});
}

void RawTextShadowNode::PaintForeground(HDC dc) {
	auto text = GetValue<ShadowNode::TextProperty>();
	auto bkColor = GetValueOrParent<ShadowNode::BackgroundProperty>();
	
	auto bkOld = GetBkColor(dc);
	{
		SetBkColor(dc, bkColor.has_value() ? bkColor.value() : GetSysColor(COLOR_WINDOW));
		auto textColor = GetValueOrParent<ShadowNode::ForegroundProperty>();
		auto tcOld = SetTextColor(dc, textColor.has_value() ? textColor.value() : GetSysColor(COLOR_WINDOWTEXT));
		HGDIOBJ oldFont{};
		if (Parent()->m_hFont) {
			oldFont = SelectObject(dc, Parent()->m_hFont);
		}
		auto textAlign = GetValueOrParent<ShadowNode::TextAlignProperty>();
		constexpr static auto DefaultTextAlign = static_cast<TextAlign>(TA_LEFT | TA_TOP | TA_NOUPDATECP);
		if (!textAlign.has_value()) {
			textAlign = DefaultTextAlign;
		}

		//auto oldAlign = SetTextAlign(dc, static_cast<UINT>(textAlign.value_or(DefaultTextAlign)));

		UINT format{};
		if (*textAlign & TextAlign::Center) {
			format |= DT_CENTER;
		}
		else if (*textAlign & TextAlign::Left) {
			format |= DT_LEFT;
		}
		else if (*textAlign & TextAlign::Right) {
			format |= DT_RIGHT;
		}
		if (*textAlign & TextAlign::Baseline) {
			// etc.
		}

		RECT r;
		GetWindowRect(Parent()->window, &r);
		r.right -= r.left;
		r.bottom -= r.top;
		r.left = 0;
		r.top = 0;
		float scale = GetDpiForWindow(Parent()->window) / 96.0f;
		//DrawText(dc, text.value().c_str(), -1, &r, 0);
		DrawTextW(dc, text->c_str(), -1, &r, format);
		//SetTextAlign(dc, oldAlign);
		if (oldFont) {
			SelectObject(dc, oldFont);
		}
		SetTextColor(dc, tcOld);
		SetBkColor(dc, bkOld);
	}
}

std::shared_ptr<ShadowNode> ButtonViewManager::Create(int64_t reactTag, int64_t rootTag, HWND rootHWnd, const winrt::Microsoft::ReactNative::JSValueObject& props) {
	return std::make_shared<ShadowNode>(CreateWindow(L"Button", L"", WS_CHILD, 0, 0, 0, 0, rootHWnd, nullptr, nullptr, nullptr), nullptr, this);
}

struct ButtonProperties : ViewManagerProperties<ButtonProperties> {
	constexpr static setter_entry_t setters[] = {
		{ "title", Set<ShadowNode::TextProperty> },
	};
};
void ButtonViewManager::UpdateProperties(int64_t reactTag, std::shared_ptr<ShadowNode> node, const winrt::Microsoft::ReactNative::JSValueObject& props) {
	for (const auto& v : props) {
		const auto& propName = v.first;
		const auto& value = v.second;

		if (auto setter = ButtonProperties::GetProperty(propName)) {
			setter->setter(node.get(), value);
			if (setter->dirtyLayout) {
				GetUIManager()->DirtyYogaNode(reactTag);
			}
		}
		else {
			ViewViewManager::UpdateProperties(reactTag, node, props);
		}
	}
}

winrt::Microsoft::ReactNative::JSValueObject ButtonViewManager::GetConstants()  {
	//auto super = static_cast<ViewViewManager&>(*this).GetConstants();
	//auto props = super["NativeProps"].MoveObject();
	//props["title"] = "string";
	//super["NativeProps"]. = props;
	//return super;

	auto view = JSValueObject {
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
				{ "title", "string" },
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

	return view;
}



std::shared_ptr<PaperUIManager> IWin32ViewManager::GetUIManager() const {
	return PaperUIManager::GetFromContext(m_context.Handle());
}

#ifdef _DEBUG



void ShadowNode::PrintNode(int level) const {
	cdbg << "VM=" << (m_vm ? typeid(*m_vm).name() : "ROOT") << "\n";
	auto indent = std::string(level * 2, ' ');
	for (auto i = 0; i < m_properties.size(); i++) {
		std::string strValue = PrintValue(i);
		if (!strValue.empty()) {
			cdbg << indent << "    \t" << magic_enum::enum_name(static_cast<typename ShadowNode::property_index_t>(i)) << " = " << strValue << "\n";
		}
	}
	for (const auto& v : m_strings) {
		cdbg << indent << "    \t" << magic_enum::enum_name(v.first) << " = " << winrt::to_string(v.second) << "\n";
	}
}
#endif

YGSize DefaultYogaSelfMeasureFunc(
	YGNodeRef node,
	float width,
	YGMeasureMode widthMode,
	float height,
	YGMeasureMode heightMode) {
	auto shadowNode = reinterpret_cast<ShadowNode*>(YGNodeGetContext(node));
	return shadowNode->Measure(width, widthMode, height, heightMode);
}

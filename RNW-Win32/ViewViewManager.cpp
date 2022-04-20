#include "pch.h"
#include "ViewViewManager.h"
#include "IWin32ViewManager.h"
#include "PaperUIManager.h"
#include <strsafe.h>

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


YGSize DefaultYogaSelfMeasureFunc(
	YGNodeRef node,
	float width,
	YGMeasureMode widthMode,
	float height,
	YGMeasureMode heightMode) {
	auto shadowNode = reinterpret_cast<ShadowNode*>(YGNodeGetContext(node));
	return shadowNode->Measure(width, widthMode, height, heightMode);
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
		if (!node->WantsMouseMove()) {
			return HTTRANSPARENT;
		}
		break;
	}
	case WM_LBUTTONDOWN: {
		if (node->GetValueOrParent<ShadowNode::OnPressProperty>()) {
			node->m_vm->EmitEvent("topClick", tag, MouseEventArgs(tag, wParam, lParam));
		}
		return 0;
	}
	case WM_MOUSEMOVE: {
		if (!node->WantsMouseMove()) {
			__noop;
		}
		else {
			if (!node->m_isMouseOver && node->GetValueOrDefault<ShadowNode::OnMouseEnterProperty>()) {
				node->m_isMouseOver = true;
				TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd };
				TrackMouseEvent(&tme);

				node->m_vm->EmitEvent("topMouseEnter", tag, MouseEventArgs(tag, wParam, lParam));

			}
			return 0;
		}
		break;
	}
	case WM_MOUSELEAVE: {
		node->m_isMouseOver = false;
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



template<typename TProperties>
struct ViewManagerProperties {
	template<typename TProperty>
	static void Set(ShadowNode* node, const winrt::Microsoft::ReactNative::JSValue& value) {
		if constexpr (std::is_same_v<typename TProperty::type, COLORREF>) {
			COLORREF colorRef{};
			if (auto argb = value.TryGetInt64()) {
				auto rgb = static_cast<uint32_t>(*argb) & 0x00ffffff; //ignore alpha
				colorRef = RGB(rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff);
			} else if (auto obj = value.TryGetObject()) {
				auto brushName = obj->at("windowsbrush").AsJSString();
				// TODO: map strings to colors: 
				// ButtonForeground
				// ButtonBackground
				// ButtonBorderBrush
				// ButtonForegroundPointerOver
				// ButtonBackgroundPointerOver
				// ButtonBorderBrushPointerOver
				static constexpr std::pair<const char*, int> color_map[] = {
					{ "ButtonForeground", COLOR_BTNTEXT },
					{ "ButtonBackground", COLOR_BTNFACE },
					{ "ButtonForegroundPointerOver", COLOR_HIGHLIGHTTEXT },
					{ "ButtonBackgroundPointerOver", COLOR_HIGHLIGHT },
					{ "ButtonForegroundPressed", COLOR_WINDOWTEXT },
					{ "ButtonBackgroundPressed", COLOR_WINDOW },
				};

				if (auto c = std::find_if(color_map, color_map + std::size(color_map),
					[&brushName](auto& e) { return brushName == e.first; }); c != std::end(color_map)) {
					colorRef = GetSysColor(c->second);
				}
				else {
					OutputDebugStringA(fmt::format("Need color mapping for color named {}\n", brushName).c_str());
					static constexpr auto SeafoamGreen = RGB(0x93, 0xe9, 0xbe);
					colorRef = SeafoamGreen;
				}
			}
			node->SetValue<TProperty>(colorRef);
		}
		else if constexpr (std::is_same_v<typename TProperty::type, uint16_t>) {
			node->SetValue<TProperty>(value.AsUInt16());
		}
		else if constexpr (std::is_same_v<typename TProperty::type, int>) {
			node->SetValue<TProperty>(value.AsInt32());
		}
		else if constexpr (std::is_same_v<typename TProperty::type, float>) {
			node->SetValue<TProperty>(value.AsSingle());
		}
		else if constexpr (std::is_same_v<typename TProperty::type, bool>) {
			node->SetValue<TProperty>(!value.IsNull() && value.AsBoolean());
		}
		else if constexpr (std::is_same_v<typename TProperty::type, std::wstring_view>) {
			auto hstr = winrt::to_hstring(value.AsString());
			node->SetValue<TProperty>(hstr.c_str());
		}
		else if constexpr (std::is_same_v<typename TProperty::type, Size>) {
			Size v;
			const auto& vArray = value.AsArray();
			for (auto i = 0; i < 2; i++) {
				v[i] = vArray.at(i).AsUInt16();
			}
			node->SetValue<TProperty>(v);
		}
		else if constexpr (std::is_same_v<typename TProperty::type, TextAlign>) {
			const auto vstr = value.AsString();
			TextAlign ta{};
			if (vstr == "center") {
				ta = TextAlign::Center;
			}
			else if (vstr == "left") {
				ta = TextAlign::Left;
			}
			else if (vstr == "right") {
				ta = TextAlign::Right;
			}
			else if (vstr == "baseline") {
				ta = TextAlign::Baseline;
			}
			node->SetValue<TProperty>(ta);
		}
		else {
			static_assert(sizeof(TProperty) == 0, "can't convert jsvalue type to property type");
		}
	}


	using setter_t = void(*)(ShadowNode*, const winrt::Microsoft::ReactNative::JSValue&);
	struct setter_entry_t {
		const char* const propertyName;
		setter_t setter;
		bool dirtyLayout;
	};

	static const setter_entry_t* GetProperty(const std::string& sv) {
		auto& s = TProperties::setters;
		if (auto found = std::find_if(s, s + std::size(s),
			[sv](const auto& e) { return e.propertyName == sv; }); found != std::end(s)) {
			return found;
		}

		return nullptr;
	}
};

struct ViewProperties : ViewManagerProperties<ViewProperties> {
	constexpr static setter_entry_t setters[] = {
		{ "backgroundColor", Set<ShadowNode::BackgroundProperty> },
		{ "color", Set<ShadowNode::ForegroundProperty> },
		{ "borderRadius", Set<ShadowNode::BorderRadiusProperty> },
		{ "borderColor", Set<ShadowNode::BorderColorProperty> },
		{ "borderWidth", Set<ShadowNode::BorderWidthProperty> },
		{ "onMouseEnter", Set<ShadowNode::OnMouseEnterProperty> },
		{ "onMouseLeave", Set<ShadowNode::OnMouseLeaveProperty> },
		//{ "onPress", Set<ShadowNode::OnPressProperty> },
		{ "onClick", Set<ShadowNode::OnPressProperty> },
		{ "text", Set<ShadowNode::TextProperty>, true },
	};
};

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

JSValueObject RawTextViewManager::GetConstants() {
	JSValueObject view{
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

	return view;
}

std::shared_ptr<ShadowNode> RawTextViewManager::Create(int64_t reactTag, int64_t rootTag, HWND rootHWnd, const winrt::Microsoft::ReactNative::JSValueObject& props) {
	return std::make_shared<RawTextShadowNode>(nullptr, m_yogaConfig, this);
}

void RawTextViewManager::UpdateProperties(int64_t reactTag, std::shared_ptr<ShadowNode> node, const winrt::Microsoft::ReactNative::JSValueObject& props) {
	for (const auto& v : props) {
		const auto& propName = v.first;
		const auto& value = v.second;

		if (auto setter = ViewProperties::GetProperty(propName)) {
			setter->setter(node.get(), value);
			if (setter->dirtyLayout) {
				GetUIManager()->DirtyYogaNode(reactTag);
			}
		}
		else {

		}
	}
}

void RawTextViewManager::UpdateLayout(ShadowNode* node, int left, int top, int width, int height) {
	auto rawNode = static_cast<RawTextShadowNode*>(node);
	rawNode->m_rc = { left, top, left + width, top + height };
}

YGMeasureFunc RawTextViewManager::GetCustomMeasureFunction() {
	return DefaultYogaSelfMeasureFunc;
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


YGMeasureFunc TextViewManager::GetCustomMeasureFunction() {
	return DefaultYogaSelfMeasureFunc;
}

std::shared_ptr<ShadowNode> TextViewManager::Create(int64_t reactTag, int64_t rootTag, HWND rootHWnd, const winrt::Microsoft::ReactNative::JSValueObject& props) {
	return ViewViewManager::Create(reactTag, rootTag, rootHWnd, props);
	//return std::make_shared<TextShadowNode>(nullptr, m_yogaConfig, this);
}

struct TextProperties : ViewManagerProperties<TextProperties> {
	constexpr static setter_entry_t setters[] = {
		{ "fontSize", Set<ShadowNode::FontSizeProperty>, true },
		{ "textAlign", Set<ShadowNode::TextAlignProperty>, true },
	};
};

void TextViewManager::UpdateProperties(int64_t reactTag, std::shared_ptr<ShadowNode> node, const winrt::Microsoft::ReactNative::JSValueObject& props) {
	for (const auto& v : props) {
		const auto& propName = v.first;
		const auto& value = v.second;

		if (auto setter = TextProperties::GetProperty(propName)) {
			setter->setter(node.get(), value);
			if (setter->dirtyLayout) {
				std::static_pointer_cast<TextShadowNode>(node)->ResetFont();
				GetUIManager()->DirtyYogaNode(reactTag);
			}
		}
		else {
			ViewViewManager::UpdateProperties(reactTag, node, props);
		}
	}
}

void TextShadowNode::ResetFont() {
	auto fontSize = GetValueOrParent<ShadowNode::FontSizeProperty>();
	if (m_hFont) {
		DeleteObject(m_hFont);
		m_hFont = nullptr;
	}
	LOGFONT lf{};
	auto scale = GetDpiForWindow(window) / 96.0f;
	lf.lfHeight = static_cast<LONG>((fontSize.has_value() ? -fontSize.value() : -12) * scale);

	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfQuality = PROOF_QUALITY;
	lf.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE;
	auto fontFamily = GetValue<ShadowNode::FontFamilyProperty>();
	StringCchCopy(lf.lfFaceName, std::size(lf.lfFaceName), fontFamily.has_value() ? fontFamily->c_str() : L"Segoe UI");

	m_hFont = CreateFontIndirectW(&lf);
}

//winrt::Microsoft::ReactNative::JSValueObject GetConstants() {}
void TextViewManager::UpdateLayout(ShadowNode* node, int left, int top, int width, int height) {
	ViewViewManager::UpdateLayout(node, left, top, width, height);
}

std::shared_ptr<PaperUIManager> IWin32ViewManager::GetUIManager() const {
	return PaperUIManager::GetFromContext(m_context.Handle());
}

YGSize TextShadowNode::Measure(float width,
	YGMeasureMode widthMode,
	float height,
	YGMeasureMode heightMode) {
	auto str = m_children[0].lock()->GetValue<ShadowNode::TextProperty>();
	if (str.has_value()) {
		auto len = static_cast<int>(str->length());
		SIZE sz{};
		auto hdc = GetDC(window);

		GetTextExtentPoint32(hdc, str.value().c_str(), len, &sz);
		float scale = GetDpiForWindow(window) / 96.0f;
		return { static_cast<float>(sz.cx) * scale , static_cast<float>(sz.cy) * scale };
	}
	return { 42,42 };
}
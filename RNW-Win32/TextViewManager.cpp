#include "pch.h"
#include "ViewViewManager.h"
#include "PaperUIManager.h"
#include <strsafe.h>

using JSValueObject = winrt::Microsoft::ReactNative::JSValueObject;

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
		{ "fontFamily", Set<ShadowNode::FontFamilyProperty>, true },
	};
};

void TextViewManager::UpdateProperties(int64_t reactTag, std::shared_ptr<ShadowNode> node, const winrt::Microsoft::ReactNative::JSValueObject& props) {
	bool dirty = false;
	for (const auto& v : props) {
		const auto& propName = v.first;
		const auto& value = v.second;

		if (auto setter = TextProperties::GetProperty(propName)) {
			setter->setter(node.get(), value);
			if (setter->dirtyLayout) {
				dirty = true;
			}
		}
	}
	ViewViewManager::UpdateProperties(reactTag, node, props);

	if (dirty) {
		std::static_pointer_cast<TextShadowNode>(node)->ResetFont();
		GetUIManager()->DirtyYogaNode(reactTag);
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


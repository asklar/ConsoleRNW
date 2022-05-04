#include "pch.h"
#include "ViewViewManager.h"
#include "PaperUIManager.h"
#include <strsafe.h>
#include "TextViewManager.h"

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

void RawTextViewManager::UpdateLayout(ShadowNode* node, float left, float top, float width, float height) {
}

std::shared_ptr<ShadowNode> TextViewManager::Create(int64_t reactTag, int64_t rootTag, HWND rootHWnd, const winrt::Microsoft::ReactNative::JSValueObject& props) {
	return ViewViewManager::Create(reactTag, rootTag, rootHWnd, props);
	//return std::make_shared<TextShadowNode>(nullptr, m_yogaConfig, this);
}

struct TextProperties : ViewManagerProperties<TextProperties> {
	constexpr static setter_entry_t setters[] = {
		{ "textAlign", Set<ShadowNode::TextAlignProperty>, true },
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
		node->CreateFont();
		GetUIManager()->DirtyYogaNode(reactTag);
	}
}

LOGFONT ShadowNode::GetLogFont() const {
	LOGFONT lf{};
	auto scale = GetScaleFactor();

	auto fontSize = GetValueOrParent<ShadowNode::FontSizeProperty>();
	lf.lfHeight = static_cast<LONG>((fontSize.has_value() ? -fontSize.value() : -12) * scale);

	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfQuality = PROOF_QUALITY;
	lf.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE;
	auto fontFamily = GetValue<ShadowNode::FontFamilyProperty>();
	StringCchCopy(lf.lfFaceName, std::size(lf.lfFaceName), fontFamily.has_value() ? fontFamily->c_str() : L"Segoe UI");
	return lf;
}


//winrt::Microsoft::ReactNative::JSValueObject GetConstants() {}
void TextViewManager::UpdateLayout(ShadowNode* node, float left, float top, float width, float height) {
	ViewViewManager::UpdateLayout(node, left, top, width, height);
}

YGSize MeasureText(HWND window, std::wstring_view str, bool withScale)
{
	SIZE sz{};
	const auto len = static_cast<int>(str.length());
	auto hdc = GetDC(window);

	Gdiplus::Graphics g(window);
	GetTextExtentPoint32(hdc, str.data(), len, &sz);
	const float scale = withScale ? GetDpiForWindow(window) / 96.0f : 1.f;
	return { static_cast<float>(sz.cx) * scale , static_cast<float>(sz.cy) * scale };
}

YGSize TextShadowNode::Measure(float width,
	YGMeasureMode widthMode,
	float height,
	YGMeasureMode heightMode) {
	auto rawTextSize = m_children[0].lock()->Measure(width, widthMode, height, heightMode);
	return rawTextSize;
}

YGSize TextShadowNode::MeasureText() const {
	return m_children[0].lock()->MeasureText();
}


YGSize RawTextShadowNode::Measure(float width,
	YGMeasureMode widthMode,
	float height,
	YGMeasureMode heightMode)
{
	auto str = GetValue<ShadowNode::TextProperty>();
	if (str.has_value())
	{
		const auto len = static_cast<int>(str->length());
		auto hdc = GetDC(window);

		Gdiplus::Graphics g(window);

		auto fontFamily = GetValueOrParentOrDefault<ShadowNode::FontFamilyProperty>(L"Segoe UI");
		auto fontSize = GetValueOrParentOrDefault<ShadowNode::FontSizeProperty>(12.f);
		Gdiplus::Font font(fontFamily.c_str(), fontSize);
		Gdiplus::RectF boundingBox{};

		// Vertical
		if (!isnan(width))
		{
			if (g.MeasureString(str->c_str(), len, &font, Gdiplus::RectF{ 0, 0, width, height }, &boundingBox) != Gdiplus::Status::Ok)
			{
				assert(false);
			}
		}
		else
		{
			g.MeasureString(str->c_str(), len, &font, { 0, 0 }, &boundingBox);
		}

		const float scale = 1.f;
		return { static_cast<float>(boundingBox.Width) * scale , static_cast<float>(boundingBox.Height) * scale };
	}
	return { 42,42 };
}

YGSize ShadowNode::MeasureText() const {
	auto str = GetValue<ShadowNode::TextProperty>();
	if (str.has_value()) {
		const auto len = static_cast<int>(str->length());
		auto hdc = GetDC(window);

		Gdiplus::Graphics g(window);

		auto fontFamily = GetValueOrParentOrDefault<ShadowNode::FontFamilyProperty>(L"Segoe UI");
		auto fontSize = GetValueOrParentOrDefault<ShadowNode::FontSizeProperty>(12.f);
		Gdiplus::Font font(fontFamily.c_str(), fontSize);
		Gdiplus::RectF boundingBox{};
		auto parentDims = RectFFromYogaNode(m_parent.lock()->yogaNode.get());
		const auto rootHwnd = PaperUIManager::GetFromContext(m_vm->m_context.Handle())->RootHWND();
		RECT grandParentRect{};
			GetWindowRect(rootHwnd, &grandParentRect);
		if (!isnan(parentDims.Width) && !isnan(parentDims.Height))
		{
			if (g.MeasureString(str->c_str(), len, &font, parentDims, &boundingBox) != Gdiplus::Status::Ok)
			{
				assert(false);
			}
		}
		else
		{
			g.MeasureString(str->c_str(), len, &font, { 0, 0 }, &boundingBox);
		}

		const float scale = 1.f;
		return { static_cast<float>(boundingBox.Width) * scale , static_cast<float>(boundingBox.Height) * scale };
	}
	return { 42,42 };

	//SIZE sz{};
	//const auto& str = this->GetValue<ShadowNode::TextProperty>();
	//if (str) {
	//	return ::MeasureText(window, str.value());
	//}
	//return { 42,42 };
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
		Gdiplus::Graphics g(dc);
		
		SetBkColor(dc, bkColor.has_value() ? bkColor.value() : GetSysColor(COLOR_WINDOW));
		auto textColor_ = GetValueOrParent<ShadowNode::ForegroundProperty>();
		auto textColor= textColor_.has_value() ? textColor_.value() : GetSysColor(COLOR_WINDOWTEXT);
		const Gdiplus::Color c(GetRValue(textColor), GetGValue(textColor), GetBValue(textColor));
		auto fontFamily = GetValueOrParentOrDefault<ShadowNode::FontFamilyProperty>(L"Segoe UI");
		auto fontSize = GetValueOrParentOrDefault<ShadowNode::FontSizeProperty>(12.f);
		Gdiplus::Font font(fontFamily.c_str(), fontSize);

		auto textAlign = GetValueOrParent<ShadowNode::TextAlignProperty>();
		constexpr static auto DefaultTextAlign = static_cast<TextAlign>(TA_LEFT | TA_TOP | TA_NOUPDATECP);
		if (!textAlign.has_value()) {
			textAlign = DefaultTextAlign;
		}

		UINT format{};
		Gdiplus::StringAlignment sa{};
		if (*textAlign & TextAlign::Center) {
			format |= DT_CENTER;
			sa = Gdiplus::StringAlignment::StringAlignmentCenter;
		}
		else if (*textAlign & TextAlign::Left) {
			format |= DT_LEFT;
			sa = Gdiplus::StringAlignment::StringAlignmentNear;
		}
		else if (*textAlign & TextAlign::Right) {
			format |= DT_RIGHT;
			sa = Gdiplus::StringAlignment::StringAlignmentFar;
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

		const Gdiplus::RectF rc(0, 0, static_cast<Gdiplus::REAL>(r.right), static_cast<Gdiplus::REAL>(r.bottom));

		const auto parentDims = RectFFromYogaNode(m_parent.lock()->yogaNode.get());

		const float scale = GetScaleFactor();
		Gdiplus::StringFormat sf{};
		sf.SetAlignment(sa);

		Gdiplus::SolidBrush brush(c);
		g.DrawString(text->c_str(), -1, &font, rc, &sf, &brush);

		//DrawText(dc, text.value().c_str(), -1, &r, 0);
//		DrawTextW(dc, text->c_str(), -1, &r, format);
		//SetTextAlign(dc, oldAlign);
		SetBkColor(dc, bkOld);
	}
}

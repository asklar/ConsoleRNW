#pragma once
#include "IWin32ViewManager.h"

#include <../../.fmt/fmt-7.1.3/include/fmt/format.h>

enum class ViewKind {
    View = 0,
    RawText = 1,
    Text = 2,
    CommonControl = 3,
	Image = 4,
};

template<typename TShadowNode = ShadowNode>
struct ViewViewManager : IWin32ViewManager {

    ViewViewManager(winrt::Microsoft::ReactNative::ReactContext ctx, ViewKind kind, YGConfigRef yogaConfig);

    static LRESULT __stdcall ViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    virtual ~ViewViewManager();

    std::shared_ptr<ShadowNode> Create(int64_t reactTag, int64_t rootTag, HWND rootHWnd, const winrt::Microsoft::ReactNative::JSValueObject& props);

    void UpdateProperties(int64_t reactTag, std::shared_ptr<ShadowNode> node, const winrt::Microsoft::ReactNative::JSValueObject& props) override;

    winrt::Microsoft::ReactNative::JSValueObject GetConstants() override;

    void UpdateLayout(ShadowNode* node, float left, float top, float width, float height) override;

	static LRESULT OnHitTest(TShadowNode* node);
	static void OnEraseBackground(const HWND& hwnd, TShadowNode* node);
	static void RaiseMouseLeave(TShadowNode* node, int64_t& tag, const WPARAM& wParam, const LPARAM& lParam);
	static void RaiseMouseEnter(TShadowNode* node, int64_t& tag, const HWND& hwnd, const WPARAM& wParam, const LPARAM& lParam);
	static void RaiseOnClick(TShadowNode* node, const int64_t& tag, const WPARAM& wParam, const LPARAM& lParam);
	
	YGMeasureFunc GetCustomMeasureFunction() const override
	{
		return TShadowNode::IsCustomMeasure ? DefaultYogaSelfMeasureFunc : nullptr;
	}
	virtual const wchar_t* GetWindowClassName() const
	{
		return TShadowNode::WindowClassName;
	}

private:
    ViewKind m_kind;
	static YGSize DefaultYogaSelfMeasureFunc(
		YGNodeRef node,
		float width,
		YGMeasureMode widthMode,
		float height,
		YGMeasureMode heightMode)
	{
		auto shadowNode = reinterpret_cast<ShadowNode*>(YGNodeGetContext(node));
		return shadowNode->Measure(width, widthMode, height, heightMode);
	}

};

struct TextViewManager : ViewViewManager<TextShadowNode> {
    TextViewManager(winrt::Microsoft::ReactNative::ReactContext ctx, YGConfigRef ycr) : ViewViewManager(ctx, ViewKind::Text, ycr) {}
    std::shared_ptr<ShadowNode> Create(int64_t reactTag, int64_t rootTag, HWND rootHWnd, const winrt::Microsoft::ReactNative::JSValueObject& props) override;
    void UpdateProperties(int64_t reactTag, std::shared_ptr<ShadowNode> node, const winrt::Microsoft::ReactNative::JSValueObject& props) override;
    void UpdateLayout(ShadowNode* node, float left, float top, float width, float height) override;
};

struct RawTextViewManager : IWin32ViewManager {
    RawTextViewManager(winrt::Microsoft::ReactNative::ReactContext ctx, YGConfigRef ycr) : IWin32ViewManager(ctx, ycr) {}
    std::shared_ptr<ShadowNode> Create(int64_t reactTag, int64_t rootTag, HWND rootHWnd, const winrt::Microsoft::ReactNative::JSValueObject& props);
    void UpdateProperties(int64_t reactTag, std::shared_ptr<ShadowNode> node, const winrt::Microsoft::ReactNative::JSValueObject& props) override;
    winrt::Microsoft::ReactNative::JSValueObject GetConstants() override;
    void UpdateLayout(ShadowNode* node, float left, float top, float width, float height) override;
};

struct TextInputViewManager : ViewViewManager<TextInputShadowNode>
{
	TextInputViewManager(winrt::Microsoft::ReactNative::ReactContext ctx, YGConfigRef ycr) : ViewViewManager(ctx, ViewKind::CommonControl, ycr) {}
	void UpdateProperties(int64_t reactTag, std::shared_ptr<ShadowNode> node, const winrt::Microsoft::ReactNative::JSValueObject& props) override;
};

struct ButtonViewManager : ViewViewManager<ButtonShadowNode> {
	ButtonViewManager(winrt::Microsoft::ReactNative::ReactContext ctx, YGConfigRef ycr) : ViewViewManager(ctx, ViewKind::CommonControl, ycr) {}
	std::shared_ptr<ShadowNode> Create(int64_t reactTag, int64_t rootTag, HWND rootHWnd, const winrt::Microsoft::ReactNative::JSValueObject& props);
	void UpdateProperties(int64_t reactTag, std::shared_ptr<ShadowNode> node, const winrt::Microsoft::ReactNative::JSValueObject& props) override;
private:
	static void SetText(ShadowNode* node, const winrt::Microsoft::ReactNative::JSValue& v);
	static LRESULT WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubClass, DWORD_PTR dwRefData);
	friend struct ButtonProperties;
};

struct ImageViewManager : ViewViewManager<ImageShadowNode> {
	ImageViewManager(winrt::Microsoft::ReactNative::ReactContext ctx, YGConfigRef ycr) : ViewViewManager(ctx, ViewKind::Image, ycr) {}
	//std::shared_ptr<ShadowNode> Create(int64_t reactTag, int64_t rootTag, HWND rootHWnd, const winrt::Microsoft::ReactNative::JSValueObject& props);
	void UpdateProperties(int64_t reactTag, std::shared_ptr<ShadowNode> node, const winrt::Microsoft::ReactNative::JSValueObject& props) override;
	winrt::Microsoft::ReactNative::JSValueObject GetConstants() override;
	//void UpdateLayout(ShadowNode* node, int left, int top, int width, int height) override;
	static void SetSource(ShadowNode* node, const winrt::Microsoft::ReactNative::JSValue& v);
};

template<typename TProperties>
struct ViewManagerProperties {
	template<typename TProperty>
	static void Set(ShadowNode* node, const winrt::Microsoft::ReactNative::JSValue& value) {
		if constexpr (std::is_same_v<typename TProperty::type, COLORREF>) {
			COLORREF colorRef{};
			if (auto argb = value.TryGetInt64()) {
				auto rgb = static_cast<uint32_t>(*argb) & 0x00ffffff; //ignore alpha
				colorRef = RGB(rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff);
			}
			else if (auto obj = value.TryGetObject()) {
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
		else if constexpr (std::is_same_v<typename TProperty::type, std::wstring>) {
			auto hstr = winrt::to_hstring(value.AsString());
			std::wstring ws = hstr.c_str();
			node->SetValue<TProperty>(ws);
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
		{ "fontSize", Set<ShadowNode::FontSizeProperty>, true },
		{ "fontFamily", Set<ShadowNode::FontFamilyProperty>, true },

	};
};

struct ImageProperties : ViewManagerProperties<ImageProperties> {
	constexpr static setter_entry_t setters[] = {
		{ "source", ImageViewManager::SetSource, true },
	};
};

YGSize DefaultYogaSelfMeasureFunc(
	YGNodeRef node,
	float width,
	YGMeasureMode widthMode,
	float height,
	YGMeasureMode heightMode);

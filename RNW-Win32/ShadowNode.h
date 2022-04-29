#pragma once
#include <Windows.h>
#include "Properties.h"
#include <../yoga/yoga/Yoga.h>
#include "IWin32ViewManager.h"
struct IWin32ViewManager;


YGSize MeasureText(HWND window, std::wstring_view str, bool withScale = true);
inline YGSize operator+(const YGSize& a, const YGSize& b) {
    return { a.width + b.width, a.height + b.height };
}
inline YGSize operator*(const YGSize& a, float f) {
    return { a.width * f, a.height * f};
}

struct ShadowNode : PropertyStorage<PropertyIndex>, SparseStorage<SparsePropertyIndex, std::wstring> {
    struct YogaNodeDeleter {
        void operator()(YGNodeRef node) {
            YGNodeFree(node);
        }
    };
#ifdef _DEBUG
    void PrintNode(int level) const;
#endif
    using YogaNodePtr = std::unique_ptr<YGNode, YogaNodeDeleter>;

    inline static YogaNodePtr make_yoga_node(YGConfigRef config) {
        YogaNodePtr result(YGNodeNewWithConfig(config));
        return result;
    }

    YogaNodePtr yogaNode{};
    
    HWND window{};

    ShadowNode(HWND w, YGConfigRef config, IWin32ViewManager* vm) : window(w), yogaNode(make_yoga_node(config)), m_vm(vm) {

    }
    ~ShadowNode() {
        DestroyWindow(window);
    }
    ShadowNode(const ShadowNode&) = delete;
    ShadowNode(ShadowNode&&) = default;

    std::weak_ptr<ShadowNode> GetParent() {
        return m_parent;
    }
    
    std::weak_ptr<ShadowNode> m_parent;
    std::vector<std::weak_ptr<ShadowNode>> m_children;
    IWin32ViewManager* m_vm{nullptr};
    
    virtual LRESULT OnPaint(HDC dc = nullptr);
    bool ImplementsPadding() const noexcept { return false; }

    using BackgroundProperty = Property<COLORREF, PropertyIndex::Background>;
    using ForegroundProperty = Property<COLORREF, PropertyIndex::Foreground>;

    using BorderRadiusProperty = Property<int, PropertyIndex::BorderRadius>;
    using BorderColorProperty = Property<COLORREF, PropertyIndex::BorderColor>;
    using BorderWidthProperty = Property<uint16_t, PropertyIndex::BorderWidth>;
    
    using OnMouseEnterProperty = Property<bool, PropertyIndex::OnMouseEnter>;
    using OnMouseLeaveProperty = Property<bool, PropertyIndex::OnMouseLeave>;
    using OnPressProperty = Property<bool, PropertyIndex::OnPress>;
    using IsMouseOverProperty = Property<bool, PropertyIndex::IsMouseOver>;

    using TextProperty = SparseProperty<SparsePropertyIndex::Text>;
    using TextAlignProperty = Property<TextAlign, PropertyIndex::TextAlign>;
    using FontSizeProperty = Property<float, PropertyIndex::FontSize>;
    using FontFamilyProperty = SparseProperty<SparsePropertyIndex::FontFamily>;
    
    template<typename TProperty>
    auto GetValue() const {
        return TProperty::Get(this);
    }

    template<typename TProperty>
    typename TProperty::type GetValueOrDefault() const {
        auto v = TProperty::Get(this);
        return v.has_value() ? v.value() : typename TProperty::type();
    }

    template<typename TProperty>
    std::optional<typename TProperty::type> GetValueOrParent() const {
        auto v = GetValue<TProperty>();
        if (auto parent = m_parent.lock(); parent && !v.has_value()) {
            return parent->GetValueOrParent<TProperty>();
        }
        return v;
    }

    template<typename TProperty>
    typename TProperty::type GetValueOrParentOrDefault(const typename TProperty::type& def = typename TProperty::type{}) const {
        auto v = GetValueOrParent<TProperty>();
        return v.has_value() ? v.value() : def;
    }

    template<typename TProperty>
    void SetValue(typename TProperty::type const& value) {
        TProperty::Set(this, value);
    }

    virtual YGSize Measure(float width,
        YGMeasureMode widthMode,
        float height,
        YGMeasureMode heightMode) {
        return {};
    }

    float GetScaleFactor() const {
        return GetDpiForWindow(window) / 96.0f;
    }

    virtual bool WantsMouseMove() { return true; }

    virtual YGSize MeasureText() const;
    LOGFONT GetLogFont() const;
    virtual void CreateFont();

    static constexpr const wchar_t* WindowClassName = L"RCTView";
    static constexpr bool IsCustomWindowClass = true;
    static constexpr bool IsCustomMeasure = false;
    static constexpr DWORD CreationStyle = WS_CHILD | WS_VISIBLE;
    static winrt::Microsoft::ReactNative::JSValueObject GetNativeProps()
    {
        return winrt::Microsoft::ReactNative::JSValueObject{
                    { "onLayout", "function" },
                    { "pointerEvents", "string" },
                    { "onClick", "function" },
                    { "onMouseEnter", "function" },
                    { "onMouseLeave", "function" },
                    { "onPress", "function" },
                    { "focusable", "boolean" },
                    { "enableFocusRing", "boolean" },
                    { "tabIndex", "number" },
        };
    }
protected:
    virtual void PaintBackground(HDC dc);
    virtual void PaintForeground(HDC dc);
};

struct ImageShadowNode : ShadowNode {
    ImageShadowNode(HWND w, YGConfigRef config, IWin32ViewManager* vm) : ShadowNode(w, config, vm) {}
    void PaintForeground(HDC dc) override;
    YGSize Measure(float width,
        YGMeasureMode widthMode,
        float height,
        YGMeasureMode heightMode) override;

    using SourceProperty = SparseProperty<SparsePropertyIndex::Source>;

    static constexpr const wchar_t* WindowClassName = L"RCTImage";
    static constexpr bool IsCustomMeasure = true;
private:
    std::unique_ptr<Gdiplus::Bitmap> m_bitmap;
    friend struct ImageViewManager;
};

struct TextShadowNode : ShadowNode {
    TextShadowNode(HWND w, YGConfigRef config, IWin32ViewManager* vm) : ShadowNode(w, config, vm) {}
    void PaintForeground(HDC dc) override;
    YGSize Measure(float width,
        YGMeasureMode widthMode,
        float height,
        YGMeasureMode heightMode) override;
    YGSize MeasureText() const override;
    bool WantsMouseMove() override { return false; }
    static constexpr const wchar_t* WindowClassName = L"RCTText";
    static constexpr bool IsCustomMeasure = true;
};
struct RawTextShadowNode : ShadowNode {
    RawTextShadowNode(HWND w, YGConfigRef config, IWin32ViewManager* vm) : ShadowNode(w, config, vm){}
    void PaintForeground(HDC dc) override;
    bool WantsMouseMove() override { return false; }
    std::shared_ptr<TextShadowNode> Parent() const {
        return std::static_pointer_cast<TextShadowNode>(m_parent.lock());
    }
    static constexpr const wchar_t* WindowClassName = L"RCTRawText";
    static constexpr bool IsCustomMeasure = true;
};

struct ButtonShadowNode : ShadowNode
{
    ButtonShadowNode(HWND w, YGConfigRef config, IWin32ViewManager* vm) : ShadowNode(w, config, vm) {}
    YGSize Measure(float width,
        YGMeasureMode widthMode,
        float height,
        YGMeasureMode heightMode) override
    {
        return MeasureText() + YGSize{ 4, 4 } * GetScaleFactor();
    }
    void CreateFont() override;
    static constexpr const wchar_t* WindowClassName = L"BUTTON";
    static constexpr bool IsCustomWindowClass = false;
    static constexpr bool IsCustomMeasure = true;
    static winrt::Microsoft::ReactNative::JSValueObject GetNativeProps()
    {
        return winrt::Microsoft::ReactNative::JSValueObject{
            { "onLayout", "function" },
            { "pointerEvents", "string" },
            { "onClick", "function" },
            { "onMouseEnter", "function" },
            { "onMouseLeave", "function" },
            { "focusable", "boolean" },
            { "enableFocusRing", "boolean" },
            { "tabIndex", "number" },
            { "title", "string" },
            { "fontFamily", "string" },
            { "fontSize", "number" },
        };
    }
};

struct TextInputShadowNode : ShadowNode
{
    TextInputShadowNode(HWND w, YGConfigRef config, IWin32ViewManager* vm) : ShadowNode(w, config, vm) {}
    YGSize Measure(float width,
        YGMeasureMode widthMode,
        float height,
        YGMeasureMode heightMode) override
    {
        return YGSize{ 150, 24 } * GetScaleFactor();// MeasureText() + YGSize{ 14, 14 } *GetScaleFactor();
    }
    static constexpr const wchar_t* WindowClassName = WC_EDITW;
    static constexpr bool IsCustomWindowClass = false;
    static constexpr bool IsCustomMeasure = true;
    static constexpr DWORD CreationStyle = ShadowNode::CreationStyle | WS_TABSTOP | WS_BORDER;
    static winrt::Microsoft::ReactNative::JSValueObject GetNativeProps()
    {
        return winrt::Microsoft::ReactNative::JSValueObject{
            { "onLayout", "function" },
            { "pointerEvents", "string" },
            { "onClick", "function" },
            { "onMouseEnter", "function" },
            { "onMouseLeave", "function" },
            { "focusable", "boolean" },
            { "enableFocusRing", "boolean" },
            { "tabIndex", "number" },
            { "value", "string" },
            { "fontFamily", "string" },
            { "fontSize", "number" },
        };
    }

};


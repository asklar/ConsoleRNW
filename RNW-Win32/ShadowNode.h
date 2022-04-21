#pragma once
#include <Windows.h>
#include "Properties.h"
#include <../yoga/yoga/Yoga.h>
#include "IWin32ViewManager.h"
struct IWin32ViewManager;

struct ShadowNode : PropertyStorage<PropertyIndex>, StringStorage<StringPropertyIndex> {
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

    using TextProperty = StringProperty<StringPropertyIndex::Text>;
    using TextAlignProperty = Property<TextAlign, PropertyIndex::TextAlign>;
    using FontSizeProperty = Property<float, PropertyIndex::FontSize>;
    using FontFamilyProperty = StringProperty<StringPropertyIndex::FontFamily>;


    template<typename TProperty>
    auto GetValue() {
        return TProperty::Get(this);
    }

    template<typename TProperty>
    typename TProperty::type GetValueOrDefault() {
        auto v = TProperty::Get(this);
        return v.has_value() ? v.value() : typename TProperty::type();
    }

    template<typename TProperty>
    std::optional<typename TProperty::type> GetValueOrParent() {
        auto v = GetValue<TProperty>();
        if (auto parent = m_parent.lock(); parent && !v.has_value()) {
            return parent->GetValueOrParent<TProperty>();
        }
        return v;
    }

    template<typename TProperty>
    typename TProperty::type GetValueOrParentOrDefault() {
        auto v = GetValueOrParent();
        return v.has_value() ? v.value() : typename TProperty::type();
    }

    template<typename TProperty>
    void SetValue(const typename TProperty::type& value) {
        TProperty::Set(this, value);
    }

    virtual YGSize Measure(float width,
        YGMeasureMode widthMode,
        float height,
        YGMeasureMode heightMode) {
        return {};
    }

    virtual bool WantsMouseMove() { return true; }
protected:
    virtual void PaintBackground(HDC dc);
    virtual void PaintForeground(HDC dc);
};

struct TextShadowNode : ShadowNode {
    TextShadowNode(HWND w, YGConfigRef config, IWin32ViewManager* vm) : ShadowNode(w, config, vm) {}
    void PaintForeground(HDC dc) override;
    YGSize Measure(float width,
        YGMeasureMode widthMode,
        float height,
        YGMeasureMode heightMode) override;
    bool WantsMouseMove() override { return false; }

    HFONT m_hFont{};
    void ResetFont();
};
struct RawTextShadowNode : ShadowNode {
    RawTextShadowNode(HWND w, YGConfigRef config, IWin32ViewManager* vm) : ShadowNode(w, config, vm){}
    void PaintForeground(HDC dc) override;
    RECT m_rc{};
    bool WantsMouseMove() override { return false; }
    std::shared_ptr<TextShadowNode> Parent() const {
        return std::static_pointer_cast<TextShadowNode>(m_parent.lock());
    }
};
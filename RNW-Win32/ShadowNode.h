#pragma once
#include <Windows.h>
#include "Properties.h"
#include <../yoga/yoga/Yoga.h>


struct ShadowNode : PropertyStorage<PropertyIndex>, StringStorage<StringPropertyIndex> {
    HWND window{};
    struct YogaNodeDeleter {
        void operator()(YGNodeRef node) {
            YGNodeFree(node);
        }
    };

    using YogaNodePtr = std::unique_ptr<YGNode, YogaNodeDeleter>;

    inline static YogaNodePtr make_yoga_node(YGConfigRef config) {
        YogaNodePtr result(YGNodeNewWithConfig(config));
        return result;
    }

    YogaNodePtr yogaNode{};
    ShadowNode(HWND w, YGConfigRef config) : window(w), yogaNode(make_yoga_node(config)) {

    }
    ~ShadowNode() {
        DestroyWindow(window);
    }
    ShadowNode(const ShadowNode&) = delete;
    ShadowNode(ShadowNode&&) = default;

    bool ImplementsPadding() const noexcept { return false; }

    using BackgroundProperty = Property<COLORREF, PropertyIndex::Background>;
    using BorderRadiusProperty = Property<int, PropertyIndex::BorderRadius>;
    using TextProperty = StringProperty<StringPropertyIndex::Text>;
    
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
    void SetValue(const typename TProperty::type& value) {
        TProperty::Set(this, value);
    }
};

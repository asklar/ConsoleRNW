#include "pch.h"
#include "YogaHelpers.h"

#if defined(_DEBUG)
int YogaLog(
	const YGConfigRef /*config*/,
	const YGNodeRef /*node*/,
	YGLogLevel /*level*/,
	const char* format,
	va_list args) {
	int len = _scprintf(format, args);
	std::string buffer(len + 1, '\0');
	vsnprintf_s(&buffer[0], len + 1, _TRUNCATE, format, args);
	buffer.resize(len);

	// OutputDebugString will truncate output around 4k, here
	// we output a 1000 characters at a time.
	int start = 0;
	while (len - start > 1000) {
		char tmp = buffer[start + 1000];
		buffer[start + 1000] = '\0';
		OutputDebugStringA(&buffer[start]);
		buffer[start + 1000] = tmp;
		start += 1000;
	}
	OutputDebugStringA(&buffer[start]);
	OutputDebugStringA("\n");

	return 0;
}
#endif

inline float NumberOrDefault(const winrt::Microsoft::ReactNative::JSValue& value, float defaultValue) {
    float result = defaultValue;

    if (value.Type() == winrt::Microsoft::ReactNative::JSValueType::Double ||
        value.Type() == winrt::Microsoft::ReactNative::JSValueType::Int64)
        result = value.AsSingle();
    else if (value.IsNull())
        result = defaultValue;
    else if (value.Type() == winrt::Microsoft::ReactNative::JSValueType::String)
        result = std::stof(value.AsString());
    else
        assert(false);

    return result;
}

inline void SetYogaValueHelper(const YGNodeRef yogaNode, const YGEdge edge, const YGValue& value, YogaSetterFunc normalSetter, YogaSetterFunc percentSetter) {
    switch (value.unit) {
    case YGUnitAuto:
    case YGUnitUndefined:
        normalSetter(yogaNode, edge, YGUndefined);
        break;
    case YGUnitPoint:
        normalSetter(yogaNode, edge, value.value);
        break;
    case YGUnitPercent:
        percentSetter(yogaNode, edge, value.value);
        break;
    }
}

inline void SetYogaUnitValueHelper(const YGNodeRef yogaNode, const YGValue& value, YogaUnitSetterFunc normalSetter, YogaUnitSetterFunc percentSetter) {
    switch (value.unit) {
    case YGUnitAuto:
    case YGUnitUndefined:
        normalSetter(yogaNode, YGUndefined);
        break;
    case YGUnitPoint:
        normalSetter(yogaNode, value.value);
        break;
    case YGUnitPercent:
        percentSetter(yogaNode, value.value);
        break;
    }
}

inline void SetYogaUnitValueAutoHelper(const YGNodeRef yogaNode, const YGValue& value, YogaUnitSetterFunc normalSetter, YogaUnitSetterFunc percentSetter, YogaAutoUnitSetterFunc autoSetter) {
    switch (value.unit) {
    case YGUnitAuto:
        autoSetter(yogaNode);
        break;
    case YGUnitUndefined:
        normalSetter(yogaNode, YGUndefined);
        break;
    case YGUnitPoint:
        normalSetter(yogaNode, value.value);
        break;
    case YGUnitPercent:
        percentSetter(yogaNode, value.value);
        break;
    }
}

inline void SetYogaValueAutoHelper(const YGNodeRef yogaNode, const YGEdge edge, const YGValue& value, YogaSetterFunc normalSetter, YogaSetterFunc percentSetter, YogaAutoSetterFunc autoSetter) {
    switch (value.unit) {
    case YGUnitAuto:
        autoSetter(yogaNode, edge);
        break;
    case YGUnitUndefined:
        normalSetter(yogaNode, edge, YGUndefined);
        break;
    case YGUnitPoint:
        normalSetter(yogaNode, edge, value.value);
        break;
    case YGUnitPercent:
        percentSetter(yogaNode, edge, value.value);
        break;
    }
}
#define consteval constexpr

namespace details {

    _NODISCARD static consteval size_t _Fnv1a_append_bytes(size_t _Val, const char* _First,
        const size_t _Count) noexcept { // accumulate range [_First, _First + _Count) into partial FNV-1a hash _Val
        for (size_t _Idx = 0; _Idx < _Count; ++_Idx) {
            _Val ^= static_cast<size_t>(_First[_Idx]);
            _Val *= std::_FNV_prime;
        }

        return _Val;
    }

    _NODISCARD static constexpr size_t _Fnv1a_append_bytes_constexpr(size_t _Val, const char* _First,
        const size_t _Count) noexcept { // accumulate range [_First, _First + _Count) into partial FNV-1a hash _Val
        for (size_t _Idx = 0; _Idx < _Count; ++_Idx) {
            _Val ^= static_cast<size_t>(_First[_Idx]);
            _Val *= std::_FNV_prime;
        }

        return _Val;
    }

    static size_t hash(const char* str) noexcept {
        return _Fnv1a_append_bytes_constexpr(std::_FNV_offset_basis, str, strlen(str));
    }

    template<size_t N>
    struct fixed_string {
        char val[N];
        consteval fixed_string(const char(&v)[N + 1]) noexcept {
            for (size_t i = 0; i < N; i++) {
                val[i] = v[i];
            }
        }
        consteval size_t length() const noexcept { return N; }

        consteval operator const char* () const noexcept { return val; }
        consteval const char& operator[](size_t pos) const {
            return val[pos];
        }
        constexpr bool operator==(const char* v) {
            return strcmp(val, v) == 0;
        }

        consteval size_t hash() const noexcept {
            return _Fnv1a_append_bytes(std::_FNV_offset_basis, val, N);
        }

    };
    template<size_t N>	 fixed_string(char const (&)[N])->fixed_string<N - 1>;

}

#define HASH_STRING(STR) details::fixed_string{ STR }.hash()


void StyleYogaNode(
    ShadowNode& shadowNode,
    const YGNodeRef yogaNode,
    const winrt::Microsoft::ReactNative::JSValueObject& props) {
    for (const auto& pair : props) {
        const auto& key = pair.first;
        const auto key_hash = details::hash(pair.first.c_str());
        const auto& value = pair.second;
        const auto value_hash = value.Type() == winrt::Microsoft::ReactNative::JSValueType::String
            ? details::hash(value.TryGetString()->c_str())
            : 0ull;

        switch (key_hash) {
        case HASH_STRING("flexDirection"): {
            YGFlexDirection direction = YGFlexDirectionColumn;

            if (value_hash == HASH_STRING("column") || value.IsNull())
                direction = YGFlexDirectionColumn;
            else if (value_hash == HASH_STRING("row"))
                direction = YGFlexDirectionRow;
            else if (value_hash == HASH_STRING("column-reverse"))
                direction = YGFlexDirectionColumnReverse;
            else if (value_hash == HASH_STRING("row-reverse"))
                direction = YGFlexDirectionRowReverse;
            else
                assert(false);

            YGNodeStyleSetFlexDirection(yogaNode, direction);
            break;
        }
        case HASH_STRING("justifyContent"): {
            YGJustify justify = YGJustifyFlexStart;

            if (value_hash == HASH_STRING("flex-start") || value.IsNull())
                justify = YGJustifyFlexStart;
            else if (value_hash == HASH_STRING("flex-end"))
                justify = YGJustifyFlexEnd;
            else if (value_hash == HASH_STRING("center"))
                justify = YGJustifyCenter;
            else if (value_hash == HASH_STRING("space-between"))
                justify = YGJustifySpaceBetween;
            else if (value_hash == HASH_STRING("space-around"))
                justify = YGJustifySpaceAround;
            else if (value_hash == HASH_STRING("space-evenly"))
                justify = YGJustifySpaceEvenly;
            else
                assert(false);

            YGNodeStyleSetJustifyContent(yogaNode, justify);
            break;
        }
        case HASH_STRING("flexWrap"): {
            YGWrap wrap = YGWrapNoWrap;

            if (value_hash == HASH_STRING("nowrap") || value.IsNull())
                wrap = YGWrapNoWrap;
            else if (value_hash == HASH_STRING("wrap"))
                wrap = YGWrapWrap;
            else if (value_hash == HASH_STRING("wrap-reverse"))
                wrap = YGWrapWrapReverse;
            else
                assert(false);

            YGNodeStyleSetFlexWrap(yogaNode, wrap);
            break;
        }
        case HASH_STRING("alignItems"): {
            YGAlign align = YGAlignStretch;

            if (value_hash == HASH_STRING("stretch") || value.IsNull())
                align = YGAlignStretch;
            else if (value_hash == HASH_STRING("flex-start"))
                align = YGAlignFlexStart;
            else if (value_hash == HASH_STRING("flex-end"))
                align = YGAlignFlexEnd;
            else if (value_hash == HASH_STRING("center"))
                align = YGAlignCenter;
            else if (value_hash == HASH_STRING("baseline"))
                align = YGAlignBaseline;
            else
                assert(false);

            YGNodeStyleSetAlignItems(yogaNode, align);
            break;
        }
        case HASH_STRING("alignSelf"): {
            YGAlign align = YGAlignAuto;

            if (value_hash == HASH_STRING("auto") || value.IsNull())
                align = YGAlignAuto;
            else if (value_hash == HASH_STRING("stretch"))
                align = YGAlignStretch;
            else if (value_hash == HASH_STRING("flex-start"))
                align = YGAlignFlexStart;
            else if (value_hash == HASH_STRING("flex-end"))
                align = YGAlignFlexEnd;
            else if (value_hash == HASH_STRING("center"))
                align = YGAlignCenter;
            else if (value_hash == HASH_STRING("baseline"))
                align = YGAlignBaseline;
            else
                assert(false);

            YGNodeStyleSetAlignSelf(yogaNode, align);
            break;
        }
        case HASH_STRING("alignContent"): {
            YGAlign align = YGAlignFlexStart;

            if (value_hash == HASH_STRING("stretch"))
                align = YGAlignStretch;
            else if (value_hash == HASH_STRING("flex-start") || value.IsNull())
                align = YGAlignFlexStart;
            else if (value_hash == HASH_STRING("flex-end"))
                align = YGAlignFlexEnd;
            else if (value_hash == HASH_STRING("center"))
                align = YGAlignCenter;
            else if (value_hash == HASH_STRING("space-between"))
                align = YGAlignSpaceBetween;
            else if (value_hash == HASH_STRING("space-around"))
                align = YGAlignSpaceAround;
            else
                assert(false);

            YGNodeStyleSetAlignContent(yogaNode, align);
            break;
        }
        case HASH_STRING("flex"): {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetFlex(yogaNode, result);
            break;
        }
        case HASH_STRING("flexGrow"): {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetFlexGrow(yogaNode, result);
            break;
        }
        case HASH_STRING("flexShrink"): {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetFlexShrink(yogaNode, result);
            break;
        }
        case HASH_STRING("flexBasis"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaUnitValueAutoHelper(
                yogaNode, result, YGNodeStyleSetFlexBasis, YGNodeStyleSetFlexBasisPercent, YGNodeStyleSetFlexBasisAuto);
            break;
        }
        case HASH_STRING("position"): {
            YGPositionType position = YGPositionTypeRelative;

            if (value_hash == HASH_STRING("relative") || value.IsNull())
                position = YGPositionTypeRelative;
            else if (value_hash == HASH_STRING("absolute"))
                position = YGPositionTypeAbsolute;
            else if (value_hash == HASH_STRING("static"))
                position = YGPositionTypeStatic;
            else
                assert(false);

            YGNodeStyleSetPositionType(yogaNode, position);
            break;
        }
        case HASH_STRING("overflow"): {
            YGOverflow overflow = YGOverflowVisible;
            if (value_hash == HASH_STRING("visible") || value.IsNull())
                overflow = YGOverflowVisible;
            else if (value_hash == HASH_STRING("hidden"))
                overflow = YGOverflowHidden;
            else if (value_hash == HASH_STRING("scroll"))
                overflow = YGOverflowScroll;

            YGNodeStyleSetOverflow(yogaNode, overflow);
            break;
        }
        case HASH_STRING("display"): {
            YGDisplay display = YGDisplayFlex;
            if (value_hash == HASH_STRING("flex") || value.IsNull())
                display = YGDisplayFlex;
            else if (value == "none")
                display = YGDisplayNone;

            YGNodeStyleSetDisplay(yogaNode, display);
            break;
        }
        case HASH_STRING("direction"): {
            // https://github.com/microsoft/react-native-windows/issues/4668
            // In order to support the direction property, we tell yoga to always layout
            // in LTR direction, then push the appropriate FlowDirection into XAML.
            // This way XAML handles flipping in RTL mode, which works both for RN components
            // as well as native components that have purely XAML sub-trees (eg ComboBox).
            YGDirection direction = YGDirectionLTR;

            YGNodeStyleSetDirection(yogaNode, direction);
            break;
        }
        case HASH_STRING("aspectRatio"): {
            float result = NumberOrDefault(value, 1.0f /*default*/);

            YGNodeStyleSetAspectRatio(yogaNode, result);
            break;
        }
        case HASH_STRING("left"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueHelper(yogaNode, YGEdgeLeft, result, YGNodeStyleSetPosition, YGNodeStyleSetPositionPercent);
            break;
        }
        case HASH_STRING("top"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueHelper(yogaNode, YGEdgeTop, result, YGNodeStyleSetPosition, YGNodeStyleSetPositionPercent);
            break;
        }
        case HASH_STRING("right"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueHelper(yogaNode, YGEdgeRight, result, YGNodeStyleSetPosition, YGNodeStyleSetPositionPercent);
            break;
        }
        case HASH_STRING("bottom"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueHelper(yogaNode, YGEdgeBottom, result, YGNodeStyleSetPosition, YGNodeStyleSetPositionPercent);
            break;
        }
        case HASH_STRING("end"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueHelper(yogaNode, YGEdgeEnd, result, YGNodeStyleSetPosition, YGNodeStyleSetPositionPercent);
            break;
        }
        case HASH_STRING("start"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueHelper(yogaNode, YGEdgeStart, result, YGNodeStyleSetPosition, YGNodeStyleSetPositionPercent);
            break;
        }
        case HASH_STRING("width"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaUnitValueAutoHelper(
                yogaNode, result, YGNodeStyleSetWidth, YGNodeStyleSetWidthPercent, YGNodeStyleSetWidthAuto);
            break;
        }
        case HASH_STRING("minWidth"): {
            YGValue result = YGValueOrDefault(value, YGValue{ 0.0f, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaUnitValueHelper(yogaNode, result, YGNodeStyleSetMinWidth, YGNodeStyleSetMinWidthPercent);
            break;
        }
        case HASH_STRING("maxWidth"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaUnitValueHelper(yogaNode, result, YGNodeStyleSetMaxWidth, YGNodeStyleSetMaxWidthPercent);
            break;
        }
        case HASH_STRING("height"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaUnitValueAutoHelper(
                yogaNode, result, YGNodeStyleSetHeight, YGNodeStyleSetHeightPercent, YGNodeStyleSetHeightAuto);
            break;
        }
        case HASH_STRING("minHeight"): {
            YGValue result = YGValueOrDefault(value, YGValue{ 0.0f, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaUnitValueHelper(yogaNode, result, YGNodeStyleSetMinHeight, YGNodeStyleSetMinHeightPercent);
            break;
        }
        case HASH_STRING("maxHeight"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaUnitValueHelper(yogaNode, result, YGNodeStyleSetMaxHeight, YGNodeStyleSetMaxHeightPercent);
            break;
        }
        case HASH_STRING("margin"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueAutoHelper(
                yogaNode, YGEdgeAll, result, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent, YGNodeStyleSetMarginAuto);
            break;
        }
        case HASH_STRING("marginLeft"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueAutoHelper(
                yogaNode, YGEdgeLeft, result, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent, YGNodeStyleSetMarginAuto);
            break;
        }
        case HASH_STRING("marginStart"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueAutoHelper(
                yogaNode, YGEdgeStart, result, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent, YGNodeStyleSetMarginAuto);
            break;
        }
        case HASH_STRING("marginTop"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueAutoHelper(
                yogaNode, YGEdgeTop, result, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent, YGNodeStyleSetMarginAuto);
            break;
        }
        case HASH_STRING("marginRight"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueAutoHelper(
                yogaNode, YGEdgeRight, result, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent, YGNodeStyleSetMarginAuto);
            break;
        }
        case HASH_STRING("marginEnd"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueAutoHelper(
                yogaNode, YGEdgeEnd, result, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent, YGNodeStyleSetMarginAuto);
            break;
        }
        case HASH_STRING("marginBottom"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueAutoHelper(
                yogaNode,
                YGEdgeBottom,
                result,
                YGNodeStyleSetMargin,
                YGNodeStyleSetMarginPercent,
                YGNodeStyleSetMarginAuto);
            break;
        }
        case HASH_STRING("marginHorizontal"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueAutoHelper(
                yogaNode,
                YGEdgeHorizontal,
                result,
                YGNodeStyleSetMargin,
                YGNodeStyleSetMarginPercent,
                YGNodeStyleSetMarginAuto);
            break;
        }
        case HASH_STRING("marginVertical"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueAutoHelper(
                yogaNode,
                YGEdgeVertical,
                result,
                YGNodeStyleSetMargin,
                YGNodeStyleSetMarginPercent,
                YGNodeStyleSetMarginAuto);
            break;
        }
        case HASH_STRING("padding"): {
            if (!shadowNode.ImplementsPadding()) {
                YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

                SetYogaValueHelper(yogaNode, YGEdgeAll, result, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
            }
            break;
        }
        case HASH_STRING("paddingLeft"): {
            if (!shadowNode.ImplementsPadding()) {
                YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

                SetYogaValueHelper(yogaNode, YGEdgeLeft, result, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
            }
            break;
        }
        case HASH_STRING("paddingStart"): {
            if (!shadowNode.ImplementsPadding()) {
                YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

                SetYogaValueHelper(yogaNode, YGEdgeStart, result, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
            }
            break;
        }
        case HASH_STRING("paddingTop"): {
            if (!shadowNode.ImplementsPadding()) {
                YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

                SetYogaValueHelper(yogaNode, YGEdgeTop, result, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
            }
            break;
        }
        case HASH_STRING("paddingRight"): {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueHelper(yogaNode, YGEdgeRight, result, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
            break;
        }
        case HASH_STRING("paddingEnd"): {
            if (!shadowNode.ImplementsPadding()) {
                YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

                SetYogaValueHelper(yogaNode, YGEdgeEnd, result, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
            }
            break;
        }
        case HASH_STRING("paddingBottom"): {
            if (!shadowNode.ImplementsPadding()) {
                YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

                SetYogaValueHelper(yogaNode, YGEdgeBottom, result, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
            }
            break;
        }
        case HASH_STRING("paddingHorizontal"): {
            if (!shadowNode.ImplementsPadding()) {
                YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

                SetYogaValueHelper(yogaNode, YGEdgeHorizontal, result, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
            }
            break;
        }
        case HASH_STRING("paddingVertical"): {
            if (!shadowNode.ImplementsPadding()) {
                YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

                SetYogaValueHelper(yogaNode, YGEdgeVertical, result, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
            }
            break;
        }
        case HASH_STRING("borderWidth"): {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetBorder(yogaNode, YGEdgeAll, result);
            break;
        }
        case HASH_STRING("borderLeftWidth"): {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetBorder(yogaNode, YGEdgeLeft, result);
            break;
        }
        case HASH_STRING("borderStartWidth"): {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetBorder(yogaNode, YGEdgeStart, result);
            break;
        }
        case HASH_STRING("borderTopWidth"): {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetBorder(yogaNode, YGEdgeTop, result);
            break;
        }
        case HASH_STRING("borderRightWidth"): {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetBorder(yogaNode, YGEdgeRight, result);
            break;
        }
        case HASH_STRING("borderEndWidth"): {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetBorder(yogaNode, YGEdgeEnd, result);
            break;
        }
        case HASH_STRING("borderBottomWidth"): {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetBorder(yogaNode, YGEdgeBottom, result);
        }
        }
    }
}

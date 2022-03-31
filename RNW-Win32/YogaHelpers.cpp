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

void StyleYogaNode(
    ShadowNode& shadowNode,
    const YGNodeRef yogaNode,
    const winrt::Microsoft::ReactNative::JSValueObject& props) {
    for (const auto& pair : props) {
        const std::string& key = pair.first;
        const auto& value = pair.second;

        if (key == "flexDirection") {
            YGFlexDirection direction = YGFlexDirectionColumn;

            if (value == "column" || value.IsNull())
                direction = YGFlexDirectionColumn;
            else if (value == "row")
                direction = YGFlexDirectionRow;
            else if (value == "column-reverse")
                direction = YGFlexDirectionColumnReverse;
            else if (value == "row-reverse")
                direction = YGFlexDirectionRowReverse;
            else
                assert(false);

            YGNodeStyleSetFlexDirection(yogaNode, direction);
        }
        else if (key == "justifyContent") {
            YGJustify justify = YGJustifyFlexStart;

            if (value == "flex-start" || value.IsNull())
                justify = YGJustifyFlexStart;
            else if (value == "flex-end")
                justify = YGJustifyFlexEnd;
            else if (value == "center")
                justify = YGJustifyCenter;
            else if (value == "space-between")
                justify = YGJustifySpaceBetween;
            else if (value == "space-around")
                justify = YGJustifySpaceAround;
            else if (value == "space-evenly")
                justify = YGJustifySpaceEvenly;
            else
                assert(false);

            YGNodeStyleSetJustifyContent(yogaNode, justify);
        }
        else if (key == "flexWrap") {
            YGWrap wrap = YGWrapNoWrap;

            if (value == "nowrap" || value.IsNull())
                wrap = YGWrapNoWrap;
            else if (value == "wrap")
                wrap = YGWrapWrap;
            else
                assert(false);

            YGNodeStyleSetFlexWrap(yogaNode, wrap);
        }
        else if (key == "alignItems") {
            YGAlign align = YGAlignStretch;

            if (value == "stretch" || value.IsNull())
                align = YGAlignStretch;
            else if (value == "flex-start")
                align = YGAlignFlexStart;
            else if (value == "flex-end")
                align = YGAlignFlexEnd;
            else if (value == "center")
                align = YGAlignCenter;
            else if (value == "baseline")
                align = YGAlignBaseline;
            else
                assert(false);

            YGNodeStyleSetAlignItems(yogaNode, align);
        }
        else if (key == "alignSelf") {
            YGAlign align = YGAlignAuto;

            if (value == "auto" || value.IsNull())
                align = YGAlignAuto;
            else if (value == "stretch")
                align = YGAlignStretch;
            else if (value == "flex-start")
                align = YGAlignFlexStart;
            else if (value == "flex-end")
                align = YGAlignFlexEnd;
            else if (value == "center")
                align = YGAlignCenter;
            else if (value == "baseline")
                align = YGAlignBaseline;
            else
                assert(false);

            YGNodeStyleSetAlignSelf(yogaNode, align);
        }
        else if (key == "alignContent") {
            YGAlign align = YGAlignFlexStart;

            if (value == "stretch")
                align = YGAlignStretch;
            else if (value == "flex-start" || value.IsNull())
                align = YGAlignFlexStart;
            else if (value == "flex-end")
                align = YGAlignFlexEnd;
            else if (value == "center")
                align = YGAlignCenter;
            else if (value == "space-between")
                align = YGAlignSpaceBetween;
            else if (value == "space-around")
                align = YGAlignSpaceAround;
            else
                assert(false);

            YGNodeStyleSetAlignContent(yogaNode, align);
        }
        else if (key == "flex") {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetFlex(yogaNode, result);
        }
        else if (key == "flexGrow") {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetFlexGrow(yogaNode, result);
        }
        else if (key == "flexShrink") {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetFlexShrink(yogaNode, result);
        }
        else if (key == "flexBasis") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaUnitValueAutoHelper(
                yogaNode, result, YGNodeStyleSetFlexBasis, YGNodeStyleSetFlexBasisPercent, YGNodeStyleSetFlexBasisAuto);
        }
        else if (key == "position") {
            YGPositionType position = YGPositionTypeRelative;

            if (value == "relative" || value.IsNull())
                position = YGPositionTypeRelative;
            else if (value == "absolute")
                position = YGPositionTypeAbsolute;
            else if (value == "static")
                position = YGPositionTypeStatic;
            else
                assert(false);

            YGNodeStyleSetPositionType(yogaNode, position);
        }
        else if (key == "overflow") {
            YGOverflow overflow = YGOverflowVisible;
            if (value == "visible" || value.IsNull())
                overflow = YGOverflowVisible;
            else if (value == "hidden")
                overflow = YGOverflowHidden;
            else if (value == "scroll")
                overflow = YGOverflowScroll;

            YGNodeStyleSetOverflow(yogaNode, overflow);
        }
        else if (key == "display") {
            YGDisplay display = YGDisplayFlex;
            if (value == "flex" || value.IsNull())
                display = YGDisplayFlex;
            else if (value == "none")
                display = YGDisplayNone;

            YGNodeStyleSetDisplay(yogaNode, display);
        }
        else if (key == "direction") {
            // https://github.com/microsoft/react-native-windows/issues/4668
            // In order to support the direction property, we tell yoga to always layout
            // in LTR direction, then push the appropriate FlowDirection into XAML.
            // This way XAML handles flipping in RTL mode, which works both for RN components
            // as well as native components that have purely XAML sub-trees (eg ComboBox).
            YGDirection direction = YGDirectionLTR;

            YGNodeStyleSetDirection(yogaNode, direction);
        }
        else if (key == "aspectRatio") {
            float result = NumberOrDefault(value, 1.0f /*default*/);

            YGNodeStyleSetAspectRatio(yogaNode, result);
        }
        else if (key == "left") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueHelper(yogaNode, YGEdgeLeft, result, YGNodeStyleSetPosition, YGNodeStyleSetPositionPercent);
        }
        else if (key == "top") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueHelper(yogaNode, YGEdgeTop, result, YGNodeStyleSetPosition, YGNodeStyleSetPositionPercent);
        }
        else if (key == "right") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueHelper(yogaNode, YGEdgeRight, result, YGNodeStyleSetPosition, YGNodeStyleSetPositionPercent);
        }
        else if (key == "bottom") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueHelper(yogaNode, YGEdgeBottom, result, YGNodeStyleSetPosition, YGNodeStyleSetPositionPercent);
        }
        else if (key == "end") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueHelper(yogaNode, YGEdgeEnd, result, YGNodeStyleSetPosition, YGNodeStyleSetPositionPercent);
        }
        else if (key == "start") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueHelper(yogaNode, YGEdgeStart, result, YGNodeStyleSetPosition, YGNodeStyleSetPositionPercent);
        }
        else if (key == "width") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaUnitValueAutoHelper(
                yogaNode, result, YGNodeStyleSetWidth, YGNodeStyleSetWidthPercent, YGNodeStyleSetWidthAuto);
        }
        else if (key == "minWidth") {
            YGValue result = YGValueOrDefault(value, YGValue{ 0.0f, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaUnitValueHelper(yogaNode, result, YGNodeStyleSetMinWidth, YGNodeStyleSetMinWidthPercent);
        }
        else if (key == "maxWidth") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaUnitValueHelper(yogaNode, result, YGNodeStyleSetMaxWidth, YGNodeStyleSetMaxWidthPercent);
        }
        else if (key == "height") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaUnitValueAutoHelper(
                yogaNode, result, YGNodeStyleSetHeight, YGNodeStyleSetHeightPercent, YGNodeStyleSetHeightAuto);
        }
        else if (key == "minHeight") {
            YGValue result = YGValueOrDefault(value, YGValue{ 0.0f, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaUnitValueHelper(yogaNode, result, YGNodeStyleSetMinHeight, YGNodeStyleSetMinHeightPercent);
        }
        else if (key == "maxHeight") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaUnitValueHelper(yogaNode, result, YGNodeStyleSetMaxHeight, YGNodeStyleSetMaxHeightPercent);
        }
        else if (key == "margin") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueAutoHelper(
                yogaNode, YGEdgeAll, result, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent, YGNodeStyleSetMarginAuto);
        }
        else if (key == "marginLeft") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueAutoHelper(
                yogaNode, YGEdgeLeft, result, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent, YGNodeStyleSetMarginAuto);
        }
        else if (key == "marginStart") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueAutoHelper(
                yogaNode, YGEdgeStart, result, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent, YGNodeStyleSetMarginAuto);
        }
        else if (key == "marginTop") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueAutoHelper(
                yogaNode, YGEdgeTop, result, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent, YGNodeStyleSetMarginAuto);
        }
        else if (key == "marginRight") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueAutoHelper(
                yogaNode, YGEdgeRight, result, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent, YGNodeStyleSetMarginAuto);
        }
        else if (key == "marginEnd") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueAutoHelper(
                yogaNode, YGEdgeEnd, result, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent, YGNodeStyleSetMarginAuto);
        }
        else if (key == "marginBottom") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueAutoHelper(
                yogaNode, YGEdgeBottom, result, YGNodeStyleSetMargin, YGNodeStyleSetMarginPercent, YGNodeStyleSetMarginAuto);
        }
        else if (key == "marginHorizontal") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueAutoHelper(
                yogaNode,
                YGEdgeHorizontal,
                result,
                YGNodeStyleSetMargin,
                YGNodeStyleSetMarginPercent,
                YGNodeStyleSetMarginAuto);
        }
        else if (key == "marginVertical") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueAutoHelper(
                yogaNode,
                YGEdgeVertical,
                result,
                YGNodeStyleSetMargin,
                YGNodeStyleSetMarginPercent,
                YGNodeStyleSetMarginAuto);
        }
        else if (key == "padding") {
            if (!shadowNode.ImplementsPadding()) {
                YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

                SetYogaValueHelper(yogaNode, YGEdgeAll, result, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
            }
        }
        else if (key == "paddingLeft") {
            if (!shadowNode.ImplementsPadding()) {
                YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

                SetYogaValueHelper(yogaNode, YGEdgeLeft, result, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
            }
        }
        else if (key == "paddingStart") {
            if (!shadowNode.ImplementsPadding()) {
                YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

                SetYogaValueHelper(yogaNode, YGEdgeStart, result, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
            }
        }
        else if (key == "paddingTop") {
            if (!shadowNode.ImplementsPadding()) {
                YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

                SetYogaValueHelper(yogaNode, YGEdgeTop, result, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
            }
        }
        else if (key == "paddingRight") {
            YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

            SetYogaValueHelper(yogaNode, YGEdgeRight, result, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
        }
        else if (key == "paddingEnd") {
            if (!shadowNode.ImplementsPadding()) {
                YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

                SetYogaValueHelper(yogaNode, YGEdgeEnd, result, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
            }
        }
        else if (key == "paddingBottom") {
            if (!shadowNode.ImplementsPadding()) {
                YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

                SetYogaValueHelper(yogaNode, YGEdgeBottom, result, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
            }
        }
        else if (key == "paddingHorizontal") {
            if (!shadowNode.ImplementsPadding()) {
                YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

                SetYogaValueHelper(yogaNode, YGEdgeHorizontal, result, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
            }
        }
        else if (key == "paddingVertical") {
            if (!shadowNode.ImplementsPadding()) {
                YGValue result = YGValueOrDefault(value, YGValue{ YGUndefined, YGUnitPoint } /*default*/, shadowNode, key);

                SetYogaValueHelper(yogaNode, YGEdgeVertical, result, YGNodeStyleSetPadding, YGNodeStyleSetPaddingPercent);
            }
        }
        else if (key == "borderWidth") {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetBorder(yogaNode, YGEdgeAll, result);
        }
        else if (key == "borderLeftWidth") {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetBorder(yogaNode, YGEdgeLeft, result);
        }
        else if (key == "borderStartWidth") {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetBorder(yogaNode, YGEdgeStart, result);
        }
        else if (key == "borderTopWidth") {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetBorder(yogaNode, YGEdgeTop, result);
        }
        else if (key == "borderRightWidth") {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetBorder(yogaNode, YGEdgeRight, result);
        }
        else if (key == "borderEndWidth") {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetBorder(yogaNode, YGEdgeEnd, result);
        }
        else if (key == "borderBottomWidth") {
            float result = NumberOrDefault(value, 0.0f /*default*/);

            YGNodeStyleSetBorder(yogaNode, YGEdgeBottom, result);
        }
    }
}


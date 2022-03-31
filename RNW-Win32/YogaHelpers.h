#pragma once
#include <../yoga/yoga/Yoga.h>
#include "ShadowNode.h"

#include <../../.fmt/fmt-7.1.3/include/fmt/format.h>

int YogaLog(
	const YGConfigRef /*config*/,
	const YGNodeRef /*node*/,
	YGLogLevel /*level*/,
	const char* format,
	va_list args);

void StyleYogaNode(
    ShadowNode& shadowNode,
    const YGNodeRef yogaNode,
    const winrt::Microsoft::ReactNative::JSValueObject& props);

float NumberOrDefault(const winrt::Microsoft::ReactNative::JSValue& value, float defaultValue);

template<typename T>
YGValue YGValueOrDefault(
    const winrt::Microsoft::ReactNative::JSValue& value,
    YGValue defaultValue,
    const T& /*unused*/,
    const std::string& key) {
    YGValue result = defaultValue;

    if (value.Type() == winrt::Microsoft::ReactNative::JSValueType::Double ||
        value.Type() == winrt::Microsoft::ReactNative::JSValueType::Int64)
        return YGValue{ value.AsSingle(), YGUnitPoint };

    if (value.IsNull())
        return defaultValue;

    if (value.Type() == winrt::Microsoft::ReactNative::JSValueType::String) {
        std::string str = value.AsString();
        if (str == "auto")
            return YGValue{ YGUndefined, YGUnitAuto };
        if (str.length() > 0 && str.back() == '%') {
            str.pop_back();
            float pct{};
            auto p = std::atof(str.c_str());
            return YGValue{ pct, YGUnitPercent };
        }
        if (str.length() > 2 && (str.compare(str.length() - 2, 2, "pt") || str.compare(str.length() - 2, 2, "px"))) {
            std::string error = fmt::format("Value '{}' for {} is invalid. Cannot be converted to YGValue. '{}' unit not needed. Simply use integer value.\n",
                value.AsString(),
                key,
                str.substr((str.length() - 2), 2));
            OutputDebugStringA(error.c_str());
            return defaultValue;
        }
    }

    std::string error = fmt::format("Value '{}'  for {} is invalid. Cannot be converted to YGValue. Did you forget the %? Otherwise, simply use integer value.\n", value.AsString(), key);
    OutputDebugStringA(error.c_str());
    return defaultValue;
}

typedef void (*YogaSetterFunc)(const YGNodeRef yogaNode, const YGEdge edge, const float value);

void SetYogaValueHelper(
    const YGNodeRef yogaNode,
    const YGEdge edge,
    const YGValue& value,
    YogaSetterFunc normalSetter,
    YogaSetterFunc percentSetter);

typedef void (*YogaUnitSetterFunc)(const YGNodeRef yogaNode, const float value);
void SetYogaUnitValueHelper(
    const YGNodeRef yogaNode,
    const YGValue& value,
    YogaUnitSetterFunc normalSetter,
    YogaUnitSetterFunc percentSetter);

typedef void (*YogaAutoUnitSetterFunc)(const YGNodeRef yogaNode);
void SetYogaUnitValueAutoHelper(
    const YGNodeRef yogaNode,
    const YGValue& value,
    YogaUnitSetterFunc normalSetter,
    YogaUnitSetterFunc percentSetter,
    YogaAutoUnitSetterFunc autoSetter);

typedef void (*YogaAutoSetterFunc)(const YGNodeRef yogaNode, const YGEdge edge);
void SetYogaValueAutoHelper(
    const YGNodeRef yogaNode,
    const YGEdge edge,
    const YGValue& value,
    YogaSetterFunc normalSetter,
    YogaSetterFunc percentSetter,
    YogaAutoSetterFunc autoSetter);


void StyleYogaNode(
    ShadowNode& shadowNode,
    const YGNodeRef yogaNode,
    const winrt::Microsoft::ReactNative::JSValueObject& props);
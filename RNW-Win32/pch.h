// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#define NOMINMAX

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <hstring.h>
#include <restrictederrorinfo.h>
#include <unknwn.h>

// Windows Header Files
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#undef GetCurrentTime

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Microsoft.ReactNative.h>
#include "JSValue.h"
#include "ReactContext.h"
#include "ReactPropertyBag.h"

#include <functional>
#include <chrono>
#include <iostream>
#include <variant>
#include <optional>

#include <../../.fmt/fmt-7.1.3/include/fmt/format.h>


struct cdbg_t {};
inline const cdbg_t& operator<<(const cdbg_t& c, const std::string_view& v) {
    OutputDebugStringA(v.data());
    return c;
}

template<typename T>
const cdbg_t& operator<<(const cdbg_t& c, const T& v) {
    auto t = std::to_string(v);
    return c << t.c_str();
}
inline const cdbg_t& operator<<(const cdbg_t& c, const std::string& v) {
    OutputDebugStringA(v.data());
    return c;
}
inline const cdbg_t& operator<<(const cdbg_t& c, const char* v) {
    OutputDebugStringA(v);
    return c;
}
inline constexpr cdbg_t cdbg{};


#include <gdiplus.h>

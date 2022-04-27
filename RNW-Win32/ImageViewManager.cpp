#include "pch.h"
#include "PaperUIManager.h"
#include "ViewViewManager.h"
#include <filesystem>
using namespace winrt::Microsoft::ReactNative;

void ImageShadowNode::PaintForeground(HDC dc) {
	Gdiplus::Graphics g(dc);
	//Gdiplus::Bitmap b(L"C:/users/asklar/source/repos/ConsoleRNW/example/react.png");
	auto w = YGNodeLayoutGetWidth(yogaNode.get());
	auto h = YGNodeLayoutGetHeight(yogaNode.get());
	g.DrawImage(m_bitmap.get(), 0.f, 0.f, w, h);
}

YGSize ImageShadowNode::Measure(float width,
	YGMeasureMode widthMode,
	float height,
	YGMeasureMode heightMode) {
	if (m_bitmap) {
		Gdiplus::SizeF size{};
		m_bitmap->GetPhysicalDimension(&size);
		return { size.Width, size.Height };
	}
	return {};
}

YGMeasureFunc ImageViewManager::GetCustomMeasureFunction() {
	return DefaultYogaSelfMeasureFunc;
}


void ImageViewManager::UpdateProperties(int64_t reactTag, std::shared_ptr<ShadowNode> node, const winrt::Microsoft::ReactNative::JSValueObject& props) {
	auto sn = std::static_pointer_cast<ImageShadowNode>(node);
	for (const auto& v : props) {
		const auto& propName = v.first;
		const auto& value = v.second;

		if (auto setter = ImageProperties::GetProperty(propName)) {
			setter->setter(node.get(), value);
			if (setter->dirtyLayout)
				GetUIManager()->DirtyYogaNode(reactTag);
		}
		else {
			if (auto setter = ViewProperties::GetProperty(propName)) {
				setter->setter(node.get(), value);
				if (setter->dirtyLayout) {
					GetUIManager()->DirtyYogaNode(reactTag);
				}
			}
		}
	}
}

winrt::Microsoft::ReactNative::JSValueObject ImageViewManager::GetConstants() {
	return JSValueObject{
	{
		{ "Constants", JSValueObject{} },
		{ "Commands", JSValueObject{} },
		{ "NativeProps", JSValueObject{
			//{ "onLayout", "function" },
			//{ "pointerEvents", "string" },
			//{ "onClick", "function" },
			//{ "onMouseEnter", "function" },
			//{ "onMouseLeave", "function" },
			//{ "onPress", "function" },
			//{ "focusable", "boolean" },
			//{ "enableFocusRing", "boolean" },
			//{ "tabIndex", "number" },
			{ "source", "Map" },
		} },
		{ "bubblingEventTypes", JSValueObject{} },
		{ "directEventTypes", JSValueObject{
			{ "topMouseEnter", JSValueObject {
				{ "registrationName", "onMouseEnter" },
			}},
			{ "topMouseLeave", JSValueObject {
				{ "registrationName", "onMouseLeave" },
			}},
			{ "topClick", JSValueObject {
				{ "registrationName", "onClick" },
			}},
			{ "topLoadStart", JSValueObject {
				{ "registrationName", "onLoadStart" }
			}},
			{ "topLoadEnd", JSValueObject {
				{ "registrationName", "onLoadEnd" }
			}},
			{ "topLoad", JSValueObject {
				{ "registrationName", "onLoad" }
			}},
			{ "topError", JSValueObject {
				{ "registrationName", "onError" }
			}},
	}
}

}
	};

}

std::filesystem::path GetCurrentProcessPath() {
	wchar_t path[MAX_PATH]{};
	GetModuleFileName(nullptr, path, ARRAYSIZE(path));
	return path;
}

void ImageViewManager::SetSource(ShadowNode* node, const winrt::Microsoft::ReactNative::JSValue& v) {
	ImageProperties::Set<ImageShadowNode::SourceProperty>(node, v);

	auto sn = static_cast<ImageShadowNode*>(node);
	
	if (v.Type() == JSValueType::String) {
		auto path = winrt::to_hstring(v.AsString());
		sn->m_bitmap = std::make_unique<Gdiplus::Bitmap>(path.c_str());
	}
	else if (auto arr = v.TryGetArray(); arr && arr->size() > 0) {
		const auto& first = (*arr)[0];
		if (auto source = first.TryGetObject()) {
			if (source->contains("uri")) {
				const auto& uri = source->at("uri");
				if (auto uriString = uri.TryGetString()) {
					auto fsPath = std::filesystem::path(*uriString);
					if (fsPath.is_relative()) {
						auto absPath = GetCurrentProcessPath().parent_path() / fsPath;
						if (std::filesystem::exists(absPath)) {
							fsPath = absPath;
						}
						else {
							sn->m_bitmap = std::make_unique<Gdiplus::Bitmap>(nullptr, fsPath.wstring().c_str());
						}
					}
					/*auto cwd = std::filesystem::current_path();
					auto absPath = std::filesystem::absolute(fsPath);*/
					auto path = fsPath.wstring();
					sn->m_bitmap = std::make_unique<Gdiplus::Bitmap>(path.c_str());
				}
			}
		}
	}
}
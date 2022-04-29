#include "pch.h"
#include "PaperUIManager.h"
#include "ViewViewManager.h"
#include <filesystem>
#include "ReactImage.h"
#include <Utils/PropertyHandlerUtils.h>
#include <shcore.h>
#include <shlwapi.h>

using namespace winrt::Microsoft::ReactNative;

#pragma comment(lib, "shlwapi.lib")

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


void ImageViewManager::UpdateProperties(int64_t reactTag, std::shared_ptr<ShadowNode> node, const winrt::Microsoft::ReactNative::JSValueObject& props) {
	bool dirty = false;
	auto sn = std::static_pointer_cast<ImageShadowNode>(node);
	for (const auto& v : props) {
		const auto& propName = v.first;
		const auto& value = v.second;

		if (propName == "source")
		{
			SetSource(node.get(), value);
		}
		else {
			if (auto setter = ViewProperties::GetProperty(propName)) {
				setter->setter(node.get(), value);
				if (setter->dirtyLayout) dirty = true;
			}
		}
	}
	if (dirty) 
		GetUIManager()->DirtyYogaNode(reactTag);
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

IStream* GetIStreamFromEmbeddedResourceUri(std::string_view sv)
{
	auto str = winrt::to_hstring(sv);
	winrt::Windows::Foundation::Uri uri(str);
	auto moduleName = uri.Host();
	auto path = uri.Path();
	// skip past the leading / slash
	std::wstring resourceName{ path.c_str() + 1 };

	auto hmodule = GetModuleHandle(moduleName != L"" ? moduleName.c_str() : nullptr);
	if (!hmodule)
	{
		if (moduleName == L"assets")
		{
			// special case, let's try with the current module
			hmodule = GetModuleHandle(nullptr);
			resourceName = L"assets" + path;
		} else
		{
			throw std::invalid_argument(fmt::format("Couldn't find module {}", winrt::to_string(moduleName)));
		}
	}

	auto resource = FindResourceW(hmodule, resourceName.c_str(), RT_RCDATA);
	if (!resource)
	{
		throw std::invalid_argument(fmt::format(
			"Couldn't find resource {} in module {}", winrt::to_string(resourceName), winrt::to_string(moduleName)));
	}

	auto hglobal = LoadResource(hmodule, resource);
	if (!hglobal)
	{
		throw std::invalid_argument(fmt::format(
			"Couldn't load resource {} in module {}", winrt::to_string(resourceName), winrt::to_string(moduleName)));
	}

	auto start = static_cast<BYTE*>(LockResource(hglobal));
	if (!start)
	{
		throw std::invalid_argument(fmt::format(
			"Couldn't lock resource {} in module {}", winrt::to_string(resourceName), winrt::to_string(moduleName)));
	}

	auto size = SizeofResource(hmodule, resource);
	if (!size)
	{
		throw std::invalid_argument(fmt::format(
			"Couldn't get size of resource {} in module {}", winrt::to_string(resourceName), winrt::to_string(moduleName)));
	}

	return SHCreateMemStream(start, size);
}

winrt::fire_and_forget ImageViewManager::SetSource(ShadowNode* node, const winrt::Microsoft::ReactNative::JSValue& v)
{
	ViewProperties::Set<ImageShadowNode::SourceProperty>(node, v);

	auto sn = static_cast<ImageShadowNode*>(node);

	if (v.Type() == JSValueType::String)
	{
		auto path = winrt::to_hstring(v.AsString());
		sn->m_bitmap = std::make_unique<Gdiplus::Bitmap>(path.c_str());
	}
	else if (auto arr = v.TryGetArray(); arr && arr->size() > 0)
	{
		auto sources{ json_type_traits<std::vector<Microsoft::ReactNative::ReactImageSource>>::parseJson(v) };
		sources[0].bundleRootPath = winrt::to_string(m_context.Handle().SettingsSnapshot().BundleRootPath());

		if (sources[0].packagerAsset && sources[0].uri.find("file://") == 0)
		{
			sources[0].uri.replace(0, 7, sources[0].bundleRootPath);
		}

		// EmitImageEvent(grid, "topLoadStart", sources[0]); // TODO


		const auto& uriString = sources[0].uri;
		if (uriString.starts_with("http://") || uriString.starts_with("https://"))
		{
			sources[0].Calculate();
			auto imgStreamOperation = Microsoft::ReactNative::GetImageStreamAsync(sources[0]);
			auto imgStream = co_await imgStreamOperation;
			winrt::com_ptr<IStream> istream;
			auto unk = imgStream.as<::IUnknown>();
			CreateStreamOverRandomAccessStream(unk.get(), IID_PPV_ARGS(&istream));

			sn->m_bitmap = std::make_unique<Gdiplus::Bitmap>(istream.get());
		}
		else if (uriString.starts_with("resource://"))
		{
			winrt::com_ptr<IStream> istream;
			istream.attach(GetIStreamFromEmbeddedResourceUri(uriString));
			sn->m_bitmap = std::make_unique<Gdiplus::Bitmap>(istream.get());
		}
		else
		{
			auto fsPath = std::filesystem::path(uriString);
			if (fsPath.is_relative())
			{
				auto absPath = GetCurrentProcessPath().parent_path() / fsPath;
				if (std::filesystem::exists(absPath))
				{
					fsPath = absPath;
				}
				else
				{
					sn->m_bitmap = std::make_unique<Gdiplus::Bitmap>(nullptr, fsPath.wstring().c_str());
				}
			}
			/*auto cwd = std::filesystem::current_path();
			auto absPath = std::filesystem::absolute(fsPath);*/
			auto path = fsPath.wstring();
			sn->m_bitmap = std::make_unique<Gdiplus::Bitmap>(path.c_str());
		}
	}
	co_return;
}
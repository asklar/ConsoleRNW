// copied from RNW
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


/// <summary>
/// From ReactCommon/react/renderer/primitives.
/// </summary>
namespace facebook::react
{
    enum class ImageResizeMode
    {
        Cover,
        Contain,
        Stretch,
        Center,
        Repeat,
    };
}

namespace Microsoft::ReactNative
{

    enum class ImageSourceType { Uri = 0, Download = 1, InlineData = 2 };
    enum class ImageSourceFormat { Bitmap = 0, Svg = 1 };

    struct ReactImageSource
    {
        std::string uri;
        std::string method;
        std::string bundleRootPath;
        std::vector<std::pair<std::string, std::string>> headers;
        double width = 0;
        double height = 0;
        double scale = 1.0;
        bool packagerAsset = false;
        ImageSourceType sourceType = ImageSourceType::Uri;
        ImageSourceFormat sourceFormat = ImageSourceFormat::Bitmap;

        void Calculate();
    };

    // Helper functions
    winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::Streams::InMemoryRandomAccessStream>
        GetImageStreamAsync(const ReactImageSource& source);
    winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::Streams::InMemoryRandomAccessStream>
        GetImageInlineDataAsync(const ReactImageSource& source);
} // namespace Microsoft::ReactNative

template <typename T>
struct json_type_traits;

template <>
struct json_type_traits<Microsoft::ReactNative::ReactImageSource>
{
    static Microsoft::ReactNative::ReactImageSource parseJson(const winrt::Microsoft::ReactNative::JSValue& json)
    {
        Microsoft::ReactNative::ReactImageSource source;
        for (auto& item : json.AsObject())
        {
            if (item.first == "uri")
                source.uri = item.second.AsString();
            else if (item.first == "method")
                source.method = item.second.AsString();
            else if (item.first == "headers")
            {
                for (auto& header : item.second.AsObject())
                {
                    source.headers.push_back(std::make_pair(header.first, header.second.AsString()));
                }
            }
            else if (item.first == "width")
                source.width = item.second.AsDouble();
            else if (item.first == "height")
                source.height = item.second.AsDouble();
            else if (item.first == "scale")
                source.scale = item.second.AsDouble();
            else if (item.first == "__packager_asset")
                source.packagerAsset = item.second.AsBoolean();
        }
        return source;
    }
};

template <>
struct json_type_traits<facebook::react::ImageResizeMode>
{
    static facebook::react::ImageResizeMode parseJson(const winrt::Microsoft::ReactNative::JSValue& json)
    {
        auto resizeMode{ facebook::react::ImageResizeMode::Contain };

        if (json == "cover")
        {
            resizeMode = facebook::react::ImageResizeMode::Cover;
        }
        else if (json == "contain")
        {
            resizeMode = facebook::react::ImageResizeMode::Contain;
        }
        else if (json == "stretch")
        {
            resizeMode = facebook::react::ImageResizeMode::Stretch;
        }
        else if (json == "center")
        {
            resizeMode = facebook::react::ImageResizeMode::Center;
        }
        else if (json == "repeat")
        {
            resizeMode = facebook::react::ImageResizeMode::Repeat;
        }

        return resizeMode;
    }
};

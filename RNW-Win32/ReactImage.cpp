#include "pch.h"
#include "ReactImage.h"
#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.Web.Http.Headers.h>


namespace winrt
{
    using namespace winrt::Windows::Storage::Streams;
    using namespace winrt::Windows::Web::Http;
}

namespace Microsoft::ReactNative
{
    winrt::Uri UriTryCreate(winrt::param::hstring const& uri)
    {
        try
        {
            return winrt::Uri(uri);
        }
        catch (...)
        {
            return winrt::Uri(nullptr);
        }
    }

    void ReactImageSource::Calculate()
    {
        winrt::Uri theUri{ UriTryCreate(winrt::to_hstring(uri)) };
        winrt::hstring scheme{ theUri ? theUri.SchemeName() : L"" };
        winrt::hstring ext{ theUri ? theUri.Extension() : L"" };

        if (((scheme == L"http") || (scheme == L"https")) && !headers.empty())
        {
            sourceType = ImageSourceType::Download;
        }
        else if (scheme == L"data")
        {
            sourceType = ImageSourceType::InlineData;
            if (uri.find("image/svg+xml;base64") != std::string::npos)
            {
                sourceFormat = ImageSourceFormat::Svg;
            }
        }
        else if (ext == L".svg" || ext == L".svgz")
        {
            sourceFormat = ImageSourceFormat::Svg;
        }
    }

    winrt::IAsyncOperation<winrt::InMemoryRandomAccessStream> GetImageStreamAsync(const ReactImageSource& source)
    {
        try
        {
            co_await winrt::resume_background();

            auto httpMethod{ source.method.empty() ? winrt::HttpMethod::Get() : winrt::HttpMethod{winrt::to_hstring(source.method)} };

            winrt::Uri uri{ winrt::to_hstring(source.uri) };
            winrt::HttpRequestMessage request{ httpMethod, uri };

            if (!source.headers.empty())
            {
                for (auto& header : source.headers)
                {
                    if (_stricmp(header.first.c_str(), "authorization") == 0)
                    {
                        request.Headers().TryAppendWithoutValidation(winrt::to_hstring(header.first), winrt::to_hstring(header.second));
                    }
                    else
                    {
                        request.Headers().Append(winrt::to_hstring(header.first), winrt::to_hstring(header.second));
                    }
                }
            }

            winrt::HttpClient httpClient;
            winrt::HttpResponseMessage response{ co_await httpClient.SendRequestAsync(request) };

            if (response && response.StatusCode() == winrt::HttpStatusCode::Ok)
            {
                winrt::IInputStream inputStream{ co_await response.Content().ReadAsInputStreamAsync() };
                winrt::InMemoryRandomAccessStream memoryStream;
                co_await winrt::RandomAccessStream::CopyAsync(inputStream, memoryStream);
                memoryStream.Seek(0);

                co_return memoryStream;
            }
        }
        catch (winrt::hresult_error const& e)
        {
            //DEBUG_HRESULT_ERROR(e);
        }

        co_return nullptr;
    }

    winrt::IAsyncOperation<winrt::InMemoryRandomAccessStream> GetImageInlineDataAsync(ReactImageSource source)
    {
        size_t start = source.uri.find(',');
        if (start == std::string::npos || start + 1 > source.uri.length())
            co_return nullptr;

        try
        {
            co_await winrt::resume_background();

            std::string_view base64String(source.uri.c_str() + start + 1, source.uri.length() - start - 1);
            auto buffer =
                winrt::Windows::Security::Cryptography::CryptographicBuffer::DecodeFromBase64String(winrt::to_hstring(base64String));

            winrt::InMemoryRandomAccessStream memoryStream;
            co_await memoryStream.WriteAsync(buffer);
            memoryStream.Seek(0);

            co_return memoryStream;
        }
        catch (winrt::hresult_error const&)
        {
            // Base64 decode failed
        }

        co_return nullptr;
    }

}
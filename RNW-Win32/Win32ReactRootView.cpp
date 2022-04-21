#include "pch.h"
#include "PaperUIManager.h"
#include "Win32ReactRootView.h"

using namespace winrt;

void Win32ReactRootView::OnLayoutChanged() {
    auto uiManager = PaperUIManager::GetFromContext(m_context.Handle());
    uiManager->DoLayout();
}

void Win32ReactRootView::Start(winrt::Microsoft::ReactNative::ReactContext context)
{
    Microsoft::ReactNative::ReactViewOptions options;
    options.ComponentName(winrt::to_hstring(m_componentName));
    auto host = winrt::Microsoft::ReactNative::ReactNativeHost::FromContext(context.Handle());
    m_viewHost = Microsoft::ReactNative::ReactCoreInjection::MakeViewHost(host, options);
    m_context = context;
    m_tag = 1;

    auto uiManager = PaperUIManager::GetFromContext(context.Handle());
    uiManager->AddMeasuredRootView(this);

    context.CallJSFunction(L"AppRegistry", L"runApplication", [/*initialProps,*/ jsMainModuleName = m_componentName, rootTag = m_tag](winrt::Microsoft::ReactNative::IJSValueWriter writer) {
        writer.WriteArrayBegin();
        writer.WriteString(winrt::to_hstring(jsMainModuleName));
        writer.WriteObjectBegin();

        winrt::Microsoft::ReactNative::JSValueObject initialProps{};
        writer.WritePropertyName(L"initialProps");
        initialProps.WriteTo(writer);

        writer.WritePropertyName(L"rootTag");
        writer.WriteInt64(rootTag);
        writer.WriteObjectEnd();
        writer.WriteArrayEnd();
        });
}

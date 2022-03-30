#pragma once
#include <NativeModules.h>
#include "Win32ReactRootView.h"
#include <../yoga/yoga/YGNode.h>
#include <../yoga/yoga/Yoga.h>

namespace Mso {
    template<typename T>
    using Functor = std::function<T>;
}
void AssertTag(bool, DWORD);

REACT_MODULE(PaperUIManager, L"UIManager")
struct PaperUIManager final : std::enable_shared_from_this<PaperUIManager> {
    PaperUIManager() {}
    PaperUIManager(const PaperUIManager&) = delete;
    ~PaperUIManager();

    REACT_INIT(Initialize)
        void Initialize(winrt::Microsoft::ReactNative::ReactContext const& reactContext) noexcept;

    REACT_CONSTANT_PROVIDER(ConstantsViaConstantsProvider)
        void ConstantsViaConstantsProvider(winrt::Microsoft::ReactNative::ReactConstantProvider& constants) noexcept;

    REACT_SYNC_METHOD(getConstantsForViewManager)
        winrt::Microsoft::ReactNative::JSValueObject getConstantsForViewManager(std::string viewManagerName) noexcept;

    // Not part of the spec, but core polyfils this on the JS side.
    REACT_SYNC_METHOD(getViewManagerConfig)
        winrt::Microsoft::ReactNative::JSValueObject getViewManagerConfig(std::string viewManagerName) noexcept;

    REACT_SYNC_METHOD(getDefaultEventTypes)
        winrt::Microsoft::ReactNative::JSValueArray getDefaultEventTypes() noexcept;

    REACT_SYNC_METHOD(lazilyLoadView)
        winrt::Microsoft::ReactNative::JSValueObject lazilyLoadView(std::string name) noexcept;

    REACT_METHOD(createView)
        void createView(
            double reactTag, // How come these cannot be int64_t?
            std::string viewName,
            double rootTag,
            winrt::Microsoft::ReactNative::JSValueObject&& props) noexcept;

    REACT_METHOD(updateView)
        void updateView(double reactTag, std::string viewName, winrt::Microsoft::ReactNative::JSValueObject&& props) noexcept;

    REACT_METHOD(focus)
        void focus(double reactTag) noexcept;

    REACT_METHOD(blur)
        void blur(double reactTag) noexcept;

    REACT_METHOD(findSubviewIn)
        void findSubviewIn(
            double reactTag,
            winrt::Microsoft::ReactNative::JSValueArray&& point,
            Mso::Functor<void(double nativeViewTag, double left, double top, double width, double height)> const
            & callback) noexcept;

    // The spec is incorrect in that it specifies a number for the command ID, but its actually a number or a string
    REACT_METHOD(dispatchViewManagerCommand)
        void dispatchViewManagerCommand(
            double reactTag,
            winrt::Microsoft::ReactNative::JSValue&& commandID,
            winrt::Microsoft::ReactNative::JSValueArray&& commandArgs) noexcept;

    REACT_METHOD(measure)
        void measure(
            double reactTag,
            Mso::Functor<void(double left, double top, double width, double height, double pageX, double pageY)> const& callback) noexcept;

    REACT_METHOD(measureInWindow)
        void measureInWindow(
            double reactTag,
            Mso::Functor<void(double x, double y, double width, double height)> const& callback) noexcept;

    REACT_METHOD(viewIsDescendantOf)
        void viewIsDescendantOf(
            double reactTag,
            double ancestorReactTag,
            Mso::Functor<void(winrt::Microsoft::ReactNative::JSValue const&)> const& callback) noexcept;

    REACT_METHOD(measureLayout)
        void measureLayout(
            double reactTag,
            double ancestorReactTag,
            Mso::Functor<void(winrt::Microsoft::ReactNative::JSValue const&)> const& errorCallback,
            Mso::Functor<void(double left, double top, double width, double height)> const& callback) noexcept;

    REACT_METHOD(measureLayoutRelativeToParent)
        void measureLayoutRelativeToParent(
            double reactTag,
            Mso::Functor<void(winrt::Microsoft::ReactNative::JSValue const&)> const& errorCallback,
            Mso::Functor<void(winrt::Microsoft::ReactNative::JSValue const&)> const& callback) noexcept;

    REACT_METHOD(setJSResponder)
        void setJSResponder(double reactTag, bool blockNativeResponder) noexcept;

    REACT_METHOD(clearJSResponder)
        void clearJSResponder() noexcept;

    REACT_METHOD(configureNextLayoutAnimation)
        void configureNextLayoutAnimation(
            winrt::Microsoft::ReactNative::JSValueObject&& config,
            Mso::Functor<void()> const& callback,
            Mso::Functor<void(winrt::Microsoft::ReactNative::JSValue const&)> const& errorCallback) noexcept;

    REACT_METHOD(removeSubviewsFromContainerWithID)
        void removeSubviewsFromContainerWithID(double containerID) noexcept;

    REACT_METHOD(replaceExistingNonRootView)
        void replaceExistingNonRootView(double reactTag, double newReactTag) noexcept;

    REACT_METHOD(removeRootView)
        void removeRootView(double reactTag) noexcept;

    REACT_METHOD(setChildren)
        void setChildren(double containerTag, winrt::Microsoft::ReactNative::JSValueArray&& reactTags) noexcept;

    REACT_METHOD(manageChildren)
        void manageChildren(
            double containerTag,
            winrt::Microsoft::ReactNative::JSValueArray&& moveFromIndices,
            winrt::Microsoft::ReactNative::JSValueArray&& moveToIndices,
            winrt::Microsoft::ReactNative::JSValueArray&& addChildReactTags,
            winrt::Microsoft::ReactNative::JSValueArray&& addAtIndices,
            winrt::Microsoft::ReactNative::JSValueArray&& removeAtIndices) noexcept;

    REACT_METHOD(setLayoutAnimationEnabledExperimental)
        void setLayoutAnimationEnabledExperimental(bool enabled) noexcept;

    REACT_METHOD(sendAccessibilityEvent)
        void sendAccessibilityEvent(double reactTag, double eventType) noexcept;

    REACT_METHOD(showPopupMenu)
        void showPopupMenu(
            double reactTag,
            winrt::Microsoft::ReactNative::JSValueArray&& items,
            Mso::Functor<void(winrt::Microsoft::ReactNative::JSValue const&)> const& error,
            Mso::Functor<void(winrt::Microsoft::ReactNative::JSValue const&)> const& success) noexcept;

    REACT_METHOD(dismissPopupMenu)
        void dismissPopupMenu() noexcept;

    REACT_METHOD(onBatchCompleted)
        void onBatchCompleted() noexcept;

    void AddMeasuredRootView(Win32ReactRootView* root);
    void DoLayout();
    void UpdateExtraLayout(int64_t tag);
    static winrt::Microsoft::ReactNative::ReactPropertyId<
        winrt::Microsoft::ReactNative::ReactNonAbiValue<std::shared_ptr<PaperUIManager>>> UIManagerProperty() {
        static winrt::Microsoft::ReactNative::ReactPropertyId<winrt::Microsoft::ReactNative::ReactNonAbiValue<std::shared_ptr<PaperUIManager>>> _value{
            L"Microsoft.ReactNative.Win32",
            L"PaperUIManager"
        };
        return _value;
    }
private:
    winrt::Microsoft::ReactNative::ReactContext m_context;

    struct YogaNodeDeleter {
        void operator()(YGNodeRef node);
    };

    using YogaNodePtr = std::unique_ptr<YGNode, YogaNodeDeleter>;

    static YogaNodePtr make_yoga_node(YGConfigRef config) {
        YogaNodePtr result(YGNodeNewWithConfig(config));
        return result;
    }

    YGConfigRef m_yogaConfig{ YGConfigNew() };

    struct ShadowNode {
        HWND window{};
        YogaNodePtr yogaNode{};
        ShadowNode(HWND w, YGConfigRef config) : window(w), yogaNode(make_yoga_node(config)) {

        }
        ShadowNode(const ShadowNode&) = delete;
        ShadowNode(ShadowNode&&) = default;

        bool ImplementsPadding() const noexcept { return false; }
    };

    std::map<int64_t, std::unique_ptr<ShadowNode>> m_nodes;

    HWND TagToHWND(int64_t);
    int64_t HWNDToTag(HWND hwnd);
    int64_t m_rootTag{};

    static void StyleYogaNode(
        ShadowNode& shadowNode,
        const YGNodeRef yogaNode,
        const winrt::Microsoft::ReactNative::JSValueObject& props);

    void DirtyYogaNode(int64_t tag);
};
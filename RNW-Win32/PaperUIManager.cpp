#include "pch.h"
#include "PaperUIManager.h"

PaperUIManager::~PaperUIManager()
{
	if (m_context)
	{
		// To make sure that we destroy UI components in UI thread.
		//m_context.UIDispatcher().Post([uiManager = std::move(m_uimanager)](){uiManager; });
	}
}

void PaperUIManager::Initialize(winrt::Microsoft::ReactNative::ReactContext const& reactContext) noexcept
{
	m_context = reactContext;
	//m_uimanager = Mso::React::GetUIManagerFromContext(reactContext.Handle());
}

void PaperUIManager::ConstantsViaConstantsProvider(winrt::Microsoft::ReactNative::ReactConstantProvider& constants) noexcept
{
	auto writer = constants.Writer();
	//m_uimanager->runForEachViewManager([&, writer](const char* viewClass) noexcept {
	//	writer.WritePropertyName(winrt::to_hstring(viewClass));
	//	WriteValue(writer, m_uimanager->getConstantsForViewManager(viewClass));
	//	});
}

winrt::Microsoft::ReactNative::JSValueObject PaperUIManager::getConstantsForViewManager(std::string viewManagerName) noexcept
{
	//auto d = m_uimanager->getConstantsForViewManager(viewManagerName);
	//auto writer = winrt::Microsoft::ReactNative::MakeJSValueTreeWriter();
	//WriteValue(writer, d);
	//return winrt::Microsoft::ReactNative::TakeJSValue(writer).MoveObject();
	return {};
}

// Not part of the spec, but core polyfils this on the JS side.
winrt::Microsoft::ReactNative::JSValueObject PaperUIManager::getViewManagerConfig(std::string viewManagerName) noexcept
{
	return getConstantsForViewManager(std::move(viewManagerName));
}

winrt::Microsoft::ReactNative::JSValueArray PaperUIManager::getDefaultEventTypes() noexcept
{
	AssertTag(false, 0x201475a1 /* tag_6fhw7 */);
	return {};
}

winrt::Microsoft::ReactNative::JSValueObject PaperUIManager::lazilyLoadView(std::string name) noexcept
{
	AssertTag(false, 0x201475a0 /* tag_6fhw6 */);
	return {};
}

void PaperUIManager::createView(
	double reactTag, // How come these cannot be int64_t?
	std::string viewName,
	double rootTag,
	winrt::Microsoft::ReactNative::JSValueObject&& props) noexcept
{
	//winrt::Microsoft::ReactNative::ReactCoreInjection::PostToUIBatchingQueue(m_context.Handle(),
	//	[uimanager = m_uimanager, reactTag = static_cast<int64_t>(reactTag), viewName = std::move(viewName), rootTag = static_cast<int64_t>(rootTag), props = std::move(props)]() mutable
	//{
	//	uimanager->createView(reactTag, std::move(viewName), rootTag, Mso::React::JSValueArgWriterToDynamic([props = std::move(props)](winrt::Microsoft::ReactNative::IJSValueWriter writer)
	//	{
	//		WriteValue(writer, std::move(props));
	//	}));
	//});
}

void PaperUIManager::updateView(double reactTag, std::string viewName, winrt::Microsoft::ReactNative::JSValueObject&& props) noexcept
{
	//winrt::Microsoft::ReactNative::ReactCoreInjection::PostToUIBatchingQueue(m_context.Handle(),
	//	[uimanager = m_uimanager, reactTag = static_cast<int64_t>(reactTag), viewName = std::move(viewName), props = std::move(props)]() mutable
	//{
	//	uimanager->updateView(reactTag, std::move(viewName), Mso::React::JSValueArgWriterToDynamic([props = std::move(props)](winrt::Microsoft::ReactNative::IJSValueWriter writer)
	//	{
	//		WriteValue(writer, std::move(props));
	//	}));
	//});
}

void PaperUIManager::focus(double reactTag) noexcept
{
	//winrt::Microsoft::ReactNative::ReactCoreInjection::PostToUIBatchingQueue(m_context.Handle(), [uimanager = m_uimanager, reactTag = static_cast<int64_t>(reactTag)]() mutable
	//{
	//	uimanager->focus(reactTag);
	//});
}

void PaperUIManager::blur(double reactTag) noexcept
{
	//winrt::Microsoft::ReactNative::ReactCoreInjection::PostToUIBatchingQueue(m_context.Handle(), [uimanager = m_uimanager, reactTag = static_cast<int64_t>(reactTag)]() mutable
	//{
	//	uimanager->blur(reactTag);
	//});
}

void PaperUIManager::findSubviewIn(
	double reactTag,
	winrt::Microsoft::ReactNative::JSValueArray&& point,
	Mso::Functor<void(double nativeViewTag, double left, double top, double width, double height)> const
	& callback) noexcept
{
	double x = point[0].AsDouble();
	double y = point[1].AsDouble();
	//winrt::Microsoft::ReactNative::ReactCoreInjection::PostToUIBatchingQueue(m_context.Handle(), [context = m_context, uimanager = m_uimanager, reactTag = static_cast<int64_t>(reactTag), x, y, callback = std::move(callback)]() mutable
	//{
	//	uimanager->findSubviewIn(reactTag, x, y, [context = std::move(context), callback = std::move(callback)](double nativeViewTag, double left, double top, double width, double height) noexcept
	//	{
	//		context.JSDispatcher().Post([callback = std::move(callback), nativeViewTag, left, top, width, height]() { callback(nativeViewTag, left, top, width, height); });
	//	});
	//});
}

// The spec is incorrect in that it specifies a number for the command ID, but its actually a number or a string
void PaperUIManager::dispatchViewManagerCommand(
	double reactTag,
	winrt::Microsoft::ReactNative::JSValue&& commandID,
	winrt::Microsoft::ReactNative::JSValueArray&& commandArgs) noexcept
{
	//winrt::Microsoft::ReactNative::ReactCoreInjection::PostToUIBatchingQueue(m_context.Handle(), [uimanager = m_uimanager, reactTag = static_cast<int64_t>(reactTag), commandID = commandID.AsString(), commandArgs = std::move(commandArgs)]() mutable
	//{
	//	uimanager->dispatchViewManagerCommand(reactTag, commandID, Mso::React::JSValueArgWriterToDynamic([commandArgs = std::move(commandArgs)](winrt::Microsoft::ReactNative::IJSValueWriter writer) noexcept
	//	{
	//		WriteValue(writer, std::move(commandArgs));
	//	}));
	//});
}

void PaperUIManager::measure(
	double reactTag,
	Mso::Functor<void(double left, double top, double width, double height, double pageX, double pageY)> const& callback) noexcept
{
	//winrt::Microsoft::ReactNative::ReactCoreInjection::PostToUIBatchingQueue(m_context.Handle(), [context = m_context, uimanager = m_uimanager, reactTag = static_cast<int64_t>(reactTag), callback = std::move(callback)]() mutable
	//{
	//	uimanager->measure(reactTag, [context = std::move(context), callback = std::move(callback)](double left, double top, double width, double height, double pageX, double pageY) noexcept
	//	{
	//		context.JSDispatcher().Post([callback = std::move(callback), left, top, width, height, pageX, pageY]() { callback(left, top, width, height, pageX, pageY); });
	//	});
	//});
}

void PaperUIManager::measureInWindow(
	double reactTag,
	Mso::Functor<void(double x, double y, double width, double height)> const& callback) noexcept
{
	//winrt::Microsoft::ReactNative::ReactCoreInjection::PostToUIBatchingQueue(m_context.Handle(), [context = m_context, uimanager = m_uimanager, reactTag = static_cast<int64_t>(reactTag), callback = std::move(callback)]() mutable
	//{
	//	uimanager->measureInWindow(reactTag, [context = std::move(context), callback = std::move(callback)](double left, double top, double width, double height) noexcept
	//	{
	//		context.JSDispatcher().Post([callback = std::move(callback), left, top, width, height]() { callback(left, top, width, height); });
	//	});
	//});
}

void PaperUIManager::viewIsDescendantOf(
	double /*reactTag*/,
	double /*ancestorReactTag*/,
	Mso::Functor<void(winrt::Microsoft::ReactNative::JSValue const&)> const& /*callback*/) noexcept
{
	AssertTag(false, 0x2014759f /* tag_6fhw5 */);
}

void PaperUIManager::measureLayout(
	double reactTag,
	double ancestorReactTag,
	Mso::Functor<void(winrt::Microsoft::ReactNative::JSValue const&)> const& errorCallback,
	Mso::Functor<void(double left, double top, double width, double height)> const& callback) noexcept
{
	//winrt::Microsoft::ReactNative::ReactCoreInjection::PostToUIBatchingQueue(m_context.Handle(), [context = m_context, uimanager = m_uimanager, reactTag = static_cast<int64_t>(reactTag), ancestorReactTag = static_cast<int64_t>(ancestorReactTag), errorCallback = std::move(errorCallback), callback = std::move(callback)]() mutable
	//{
	//	uimanager->measureLayout(reactTag, ancestorReactTag,
	//		[context = std::move(context), errorCallback = std::move(errorCallback)]() noexcept
	//	{
	//		context.JSDispatcher().Post([errorCallback = std::move(errorCallback)]() { errorCallback({}); });
	//	},
	//		[context = std::move(context), callback = std::move(callback)](double left, double top, double width, double height) noexcept
	//	{
	//		context.JSDispatcher().Post([callback = std::move(callback), left, top, width, height]() { callback(left, top, width, height); });
	//	});
	//});
}

void PaperUIManager::measureLayoutRelativeToParent(
	double /*reactTag*/,
	Mso::Functor<void(winrt::Microsoft::ReactNative::JSValue const&)> const& /*errorCallback*/,
	Mso::Functor<void(winrt::Microsoft::ReactNative::JSValue const&)> const& /*callback*/) noexcept
{
	AssertTag(false, 0x2014759e /* tag_6fhw4 */);
}

void PaperUIManager::setJSResponder(double /*reactTag*/, bool /*blockNativeResponder*/) noexcept
{
}

void PaperUIManager::clearJSResponder() noexcept
{
}

void PaperUIManager::configureNextLayoutAnimation(
	winrt::Microsoft::ReactNative::JSValueObject&& config,
	Mso::Functor<void()> const& callback,
	Mso::Functor<void(winrt::Microsoft::ReactNative::JSValue const&)> const& errorCallback) noexcept
{
	//winrt::Microsoft::ReactNative::ReactCoreInjection::PostToUIBatchingQueue(m_context.Handle(),
	//	[uimanager = m_uimanager, config = std::move(config), callback = std::move(callback), errorCallback = std::move(errorCallback)]() mutable
	//{
	//	uimanager->configureNextLayoutAnimation(Mso::React::JSValueArgWriterToDynamic(
	//		[config = std::move(config)](winrt::Microsoft::ReactNative::IJSValueWriter writer)
	//	{
	//		WriteValue(writer, std::move(config));
	//	}),
	//		std::move(callback),
	//		[errorCallback = std::move(errorCallback)]() noexcept { errorCallback({}); });
	//});
}

void PaperUIManager::removeSubviewsFromContainerWithID(double containerID) noexcept
{
	//winrt::Microsoft::ReactNative::ReactCoreInjection::PostToUIBatchingQueue(m_context.Handle(), [uimanager = m_uimanager, containerID = static_cast<int64_t>(containerID)]() mutable
	//{
	//	uimanager->removeSubviewsFromContainerWithID(containerID);
	//});
}

void PaperUIManager::replaceExistingNonRootView(double reactTag, double newReactTag) noexcept
{
	//winrt::Microsoft::ReactNative::ReactCoreInjection::PostToUIBatchingQueue(m_context.Handle(), [uimanager = m_uimanager, reactTag = static_cast<int64_t>(reactTag), newReactTag = static_cast<int64_t>(newReactTag)]() mutable
	//{
	//	uimanager->replaceExistingNonRootView(reactTag, newReactTag);
	//});
}

void PaperUIManager::removeRootView(double reactTag) noexcept
{
	//winrt::Microsoft::ReactNative::ReactCoreInjection::PostToUIBatchingQueue(m_context.Handle(), [uimanager = m_uimanager, reactTag = static_cast<int64_t>(reactTag)]() mutable
	//{
	//	uimanager->removeRootView(reactTag);
	//});
}

void PaperUIManager::setChildren(double containerTag, winrt::Microsoft::ReactNative::JSValueArray&& reactTags) noexcept
{
	//winrt::Microsoft::ReactNative::ReactCoreInjection::PostToUIBatchingQueue(m_context.Handle(), [uimanager = m_uimanager, containerTag = static_cast<int64_t>(containerTag), reactTags = std::move(reactTags)]() mutable
	//{
	//	uimanager->setChildren(containerTag, Mso::React::JSValueArgWriterToDynamic([reactTags = std::move(reactTags)](winrt::Microsoft::ReactNative::IJSValueWriter writer)
	//	{
	//		WriteValue(writer, std::move(reactTags));
	//	}));
	//});
}

void PaperUIManager::manageChildren(
	double containerTag,
	winrt::Microsoft::ReactNative::JSValueArray&& moveFromIndices,
	winrt::Microsoft::ReactNative::JSValueArray&& moveToIndices,
	winrt::Microsoft::ReactNative::JSValueArray&& addChildReactTags,
	winrt::Microsoft::ReactNative::JSValueArray&& addAtIndices,
	winrt::Microsoft::ReactNative::JSValueArray&& removeAtIndices) noexcept
{
	//winrt::Microsoft::ReactNative::ReactCoreInjection::PostToUIBatchingQueue(m_context.Handle(), [
	//	uimanager = m_uimanager,
	//		containerTag = static_cast<int64_t>(containerTag),
	//		moveFromIndices = Mso::React::JSValueArgWriterToDynamic([moveFromIndices = std::move(moveFromIndices)](winrt::Microsoft::ReactNative::IJSValueWriter writer)
	//	{
	//		WriteValue(writer, std::move(moveFromIndices));
	//	}),
	//		moveToIndices = Mso::React::JSValueArgWriterToDynamic([moveToIndices = std::move(moveToIndices)](winrt::Microsoft::ReactNative::IJSValueWriter writer)
	//	{
	//		WriteValue(writer, std::move(moveToIndices));
	//	}),
	//		addChildReactTags = Mso::React::JSValueArgWriterToDynamic([addChildReactTags = std::move(addChildReactTags)](winrt::Microsoft::ReactNative::IJSValueWriter writer)
	//	{
	//		WriteValue(writer, std::move(addChildReactTags));
	//	}),
	//		addAtIndices = Mso::React::JSValueArgWriterToDynamic([addAtIndices = std::move(addAtIndices)](winrt::Microsoft::ReactNative::IJSValueWriter writer)
	//	{
	//		WriteValue(writer, std::move(addAtIndices));
	//	}),
	//		removeAtIndices = Mso::React::JSValueArgWriterToDynamic([removeAtIndices = std::move(removeAtIndices)](winrt::Microsoft::ReactNative::IJSValueWriter writer)
	//	{
	//		WriteValue(writer, std::move(removeAtIndices));
	//	})]() mutable
	//	{
	//		uimanager->manageChildren(containerTag, moveFromIndices, moveToIndices, addChildReactTags, addAtIndices, removeAtIndices);
	//	});
}

void PaperUIManager::setLayoutAnimationEnabledExperimental(bool /*enabled*/) noexcept
{
	AssertTag(false, 0x2014759d /* tag_6fhw3 */);
}

void PaperUIManager::sendAccessibilityEvent(double /*reactTag*/, double /*eventType*/) noexcept
{
	AssertTag(false, 0x2014759c /* tag_6fhw2 */);
}

void PaperUIManager::showPopupMenu(
	double /*reactTag*/,
	winrt::Microsoft::ReactNative::JSValueArray&& /*items*/,
	Mso::Functor<void(winrt::Microsoft::ReactNative::JSValue const&)> const& /*error*/,
	Mso::Functor<void(winrt::Microsoft::ReactNative::JSValue const&)> const& /*success*/) noexcept
{
	AssertTag(false, 0x2014759b /* tag_6fhw1 */);
}

void PaperUIManager::dismissPopupMenu() noexcept
{
	AssertTag(false, 0x2014759a /* tag_6fhw0 */);
}

#include "pch.h"
#include "PaperUIManager.h"
#include "ViewViewManager.h"
#include "YogaHelpers.h"

void AssertTag(bool, DWORD) {}
using namespace winrt::Microsoft::ReactNative;


PaperUIManager::~PaperUIManager()
{
}


void PaperUIManager::Initialize(winrt::Microsoft::ReactNative::ReactContext const& reactContext) noexcept
{
	m_context = reactContext;
	winrt::Microsoft::ReactNative::ReactPropertyBag(m_context.Properties())
		.Set(PaperUIManager::UIManagerProperty(), this);

#if defined(_DEBUG)
	YGConfigSetLogger(m_yogaConfig, &YogaLog);

	// To Debug Yoga layout, uncomment the following line.
	// YGConfigSetPrintTreeFlag(m_yogaConfig, true);

	// Additional logging can be enabled editing yoga.cpp (e.g. gPrintChanges,
	// gPrintSkips)
#endif

	winrt::Microsoft::ReactNative::ReactCoreInjection::SetUIBatchCompleteCallback(m_context.Properties().Handle(), 
		[](winrt::Microsoft::ReactNative::IReactPropertyBag bag) {
			auto uimgr = *winrt::Microsoft::ReactNative::ReactPropertyBag(bag).Get(UIManagerProperty());
			uimgr->onBatchCompleted();
		});
}

void PaperUIManager::ConstantsViaConstantsProvider(winrt::Microsoft::ReactNative::ReactConstantProvider& constants) noexcept
{}


void PaperUIManager::EnsureViewManager(const std::string& viewManagerName) {
	if (m_viewManagers.count(viewManagerName) == 0) {

		struct ViewManagerFactory {
			std::string_view name;
			std::unique_ptr<IWin32ViewManager>(*make)(winrt::Microsoft::ReactNative::ReactContext, YGConfigRef yogaConfig);
		};
		static constexpr const ViewManagerFactory viewMgrFactory[] = {
			{ "RCTView", [](winrt::Microsoft::ReactNative::ReactContext context, YGConfigRef yogaConfig) { return std::unique_ptr<IWin32ViewManager>(new ViewViewManager(context, ViewKind::View, yogaConfig)); }},
			{ "RCTVirtualText", [](winrt::Microsoft::ReactNative::ReactContext context, YGConfigRef yogaConfig) { return std::unique_ptr<IWin32ViewManager>(new ViewViewManager(context, ViewKind::View, yogaConfig)); }},
			{ "RCTRawText", [](winrt::Microsoft::ReactNative::ReactContext context, YGConfigRef yogaConfig) { return std::unique_ptr<IWin32ViewManager>(new RawTextViewManager(context, yogaConfig)); }},
			{ "RCTText", [](winrt::Microsoft::ReactNative::ReactContext context, YGConfigRef yogaConfig) { return std::unique_ptr<IWin32ViewManager>(new TextViewManager(context, yogaConfig)); }},
			{ "Text", [](winrt::Microsoft::ReactNative::ReactContext context, YGConfigRef yogaConfig) { return std::unique_ptr<IWin32ViewManager>(new TextViewManager(context, yogaConfig)); }},
			{ "NativeButton", [](winrt::Microsoft::ReactNative::ReactContext context, YGConfigRef ycr) { return std::unique_ptr<IWin32ViewManager>(new ButtonViewManager(context, ycr)); }},
			{ "RCTImageView", [](winrt::Microsoft::ReactNative::ReactContext context, YGConfigRef ycr) { return std::unique_ptr<IWin32ViewManager>(new ImageViewManager(context, ycr)); }},
			{ "Image", [](winrt::Microsoft::ReactNative::ReactContext context, YGConfigRef ycr) { return std::unique_ptr<IWin32ViewManager>(new ImageViewManager(context, ycr)); }},
		};
		const auto& entry = std::find_if(std::begin(viewMgrFactory), std::end(viewMgrFactory), [&viewManagerName](const auto& i) { return i.name == viewManagerName; });
		m_viewManagers[viewManagerName] = entry->make(m_context, m_yogaConfig);

	}
}

winrt::Microsoft::ReactNative::JSValueObject PaperUIManager::getConstantsForViewManager(std::string viewManagerName) noexcept
{
	EnsureViewManager(viewManagerName);

    return m_viewManagers[viewManagerName]->GetConstants();
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

void PaperUIManager::onBatchCompleted() noexcept {
	DoLayout();
	//if (m_inBatch) {
	//	m_inBatch = false;

	//	const auto callbacks = m_batchCompletedCallbacks;
	//	m_batchCompletedCallbacks.clear();
	//	for (const auto& callback : callbacks) {
	//		callback.operator()();
	//	}
	//}
}


void PaperUIManager::DirtyYogaNode(int64_t tag) {
	OutputDebugStringA(fmt::format("Dirty {}\n", tag).c_str());
    auto& pShadowNode = m_nodes[tag];
	if (pShadowNode != nullptr) {
		if (auto* pViewManager = pShadowNode->m_vm) {
			if (YGMeasureFunc func = pViewManager->GetCustomMeasureFunction()) {
				// If there is a yoga node for this tag mark it as dirty
				YGNodeRef yogaNode = pShadowNode->yogaNode.get();
				if (yogaNode != nullptr) {
					// Retrieve and dirty the yoga node
					YGNodeMarkDirty(yogaNode);

					// Once we mark a node dirty we can stop because the yoga code will mark
					// all parents anyway
					return;
				}
			}
		}
        // Since this node didn't meet the criteria, jump to parent in case it does
		auto parent_weak = pShadowNode->m_parent;
		if (auto parent = parent_weak.lock())
			DirtyYogaNode(IWin32ViewManager::GetTag(parent->window));
    }
}

void PaperUIManager::UpdateExtraLayout(int64_t tag) {
	// For nodes that are not self-measure, there may be styles applied that are
	// applying padding. Here we make sure Yoga knows about that padding so yoga
	// layout is aware of what rendering intends to do with it.  (net: buttons
	// with padding shouldn't have clipped content anymore)

/*
	auto& shadowNode = m_nodes[tag];

	if (shadowNode->IsExternalLayoutDirty()) {
		YGNodeRef yogaNode = GetYogaNode(tag);
		if (yogaNode)
			shadowNode->DoExtraLayoutPrep(yogaNode);
	}

	for (int64_t child : shadowNode->m_children) {
		UpdateExtraLayout(child);
	}
	*/
}

void PaperUIManager::DoLayout() {
	
	// Process vector of RN controls needing extra layout here.
  const auto extraLayoutNodes = m_extraLayoutNodes;
  for (const int64_t tag : extraLayoutNodes) {
    if (auto node = m_nodes[tag]) {
      auto element = node->window;
      // TODO: Measure
    }
  }
  // Values need to be cleared from the vector before next call to DoLayout.
  m_extraLayoutNodes.clear();

  

  // auto &rootTags = m_host->GetAllRootTags();
	std::vector<int64_t> rootTags{ m_rootTag };
  for (int64_t rootTag : rootTags) {
    UpdateExtraLayout(rootTag);

	const auto& rootShadowNode = m_nodes[rootTag];
	YGNodeRef rootNode = rootShadowNode->yogaNode.get();
	RECT rect{};
	GetWindowRect(rootShadowNode->window, &rect);
	float actualWidth = static_cast<float>(rect.right - rect.left);
    float actualHeight = static_cast<float>(rect.bottom - rect.top);

    // We must always run layout in LTR mode, which might seem unintuitive.
    // We will flip the root of the tree into RTL by forcing the root XAML node's FlowDirection to RightToLeft
    // which will inherit down the XAML tree, allowing all native controls to pick it up.
    YGNodeCalculateLayout(rootNode, actualWidth, actualHeight, YGDirectionLTR);
  }

  for (auto &tagToYogaNode : m_nodes) {
    int64_t tag = tagToYogaNode.first;
	if (tag == m_rootTag) continue;
    YGNodeRef yogaNode = tagToYogaNode.second->yogaNode.get();

    if (!YGNodeGetHasNewLayout(yogaNode))
      continue;
    YGNodeSetHasNewLayout(yogaNode, false);

    auto left = YGNodeLayoutGetLeft(yogaNode);
	auto top = YGNodeLayoutGetTop(yogaNode);
	auto width = YGNodeLayoutGetWidth(yogaNode);
	auto height = YGNodeLayoutGetHeight(yogaNode);

	const auto &shadowNode = m_nodes[tag];
    
	/*auto view = shadowNode.GetView();
    auto pViewManager = shadowNode.GetViewManager();
    pViewManager->SetLayoutProps(shadowNode, view, left, top, width, height);
	*/
	/// TODO
	if (width == std::numeric_limits<int>::min() ||
		height == std::numeric_limits<int>::min()) {
		// no size
	}
	else {
		shadowNode->m_vm->UpdateLayout(shadowNode.get(), left, top, width, height);
		
	}
  }
}

void PaperUIManager::createView(
	double reactTag, // How come these cannot be int64_t?
	std::string viewName,
	double rootTag,
	winrt::Microsoft::ReactNative::JSValueObject&& props) noexcept
{
	OutputDebugStringA(fmt::format("{} createView {} {}\n", GetTickCount64(), viewName, reactTag).c_str());

    winrt::Microsoft::ReactNative::ReactCoreInjection::PostToUIBatchingQueue(m_context.Handle(),
        [this, reactTag = static_cast<int64_t>(reactTag), viewName = std::move(viewName), rootTag = static_cast<int64_t>(rootTag), props = std::move(props)]() mutable
    {
		EnsureViewManager(viewName);
		const auto& vm = m_viewManagers[viewName];
        auto shadowNode = vm->Create(reactTag, rootTag, TagToHWND(rootTag), props);
#ifdef _DEBUG
		assert(!shadowNode->window || IWin32ViewManager::GetTag(shadowNode->window) != 0);
#endif

        auto result = m_nodes.emplace(reactTag, shadowNode);
        if (result.second) {
            vm->SetShadowNode(shadowNode->window, shadowNode.get());

            StyleYogaNode(*shadowNode, shadowNode->yogaNode.get(), props);

			if (auto func = vm->GetCustomMeasureFunction()) {
                YGNodeSetMeasureFunc(shadowNode->yogaNode.get(), func);

				YGNodeSetContext(shadowNode->yogaNode.get(), reinterpret_cast<void*>(shadowNode.get()));
            }
        }
        vm->UpdateProperties(reactTag, shadowNode, props);
		Invalidate(shadowNode->window);
    });
	
}

void PaperUIManager::updateView(double reactTag, std::string viewName, winrt::Microsoft::ReactNative::JSValueObject&& props) noexcept
{
    auto tag = static_cast<int64_t>(reactTag);
	auto shadowNode = m_nodes[tag];
    m_viewManagers[viewName]->UpdateProperties(tag, shadowNode, props);
	StyleYogaNode(*shadowNode, shadowNode->yogaNode.get(), props);
	DoLayout();
	Invalidate(shadowNode->window);
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
	RECT rect{};
	GetWindowRect(m_nodes[static_cast<int64_t>(reactTag)]->window, &rect);
	callback(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, rect.left, rect.top);
	//winrt::Microsoft::ReactNative::ReactCoreInjection::PostToUIBatchingQueue(m_context.Handle(), [context = m_context, reactTag = static_cast<int64_t>(reactTag), callback = std::move(callback)]() mutable
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
	RECT rect{};
	GetWindowRect(m_nodes[static_cast<int64_t>(reactTag)]->window, &rect);
	callback(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
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
	errorCallback({ "not implemented" });
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

int64_t PaperUIManager::HWNDToTag(HWND hwnd) const {
	return IWin32ViewManager::GetTag(hwnd);
}

HWND PaperUIManager::TagToHWND(int64_t tag) const {
	return m_nodes.count(tag) != 0 ? m_nodes.at(tag)->window : 0;
}

void PaperUIManager::setChildren(double containerTag, winrt::Microsoft::ReactNative::JSValueArray&& reactTags) noexcept
{
	winrt::Microsoft::ReactNative::ReactCoreInjection::PostToUIBatchingQueue(m_context.Handle(), [this, containerTag = static_cast<int64_t>(containerTag), reactTags = std::move(reactTags)]() mutable
	{
		auto parentNode = m_nodes[containerTag];

		auto isNativeControlWithSelfLayout = parentNode->m_vm && parentNode->m_vm->GetCustomMeasureFunction() != nullptr;
		
		uint32_t index = 0;
		for (auto const& child : reactTags) {
			auto childTag = child.AsInt64();
			auto node = m_nodes[childTag];
			auto oldParent = node->m_parent.lock();
			//auto oldParentHWND = oldParent ? oldParent->window : nullptr;
			node->m_parent = parentNode;
			parentNode->m_children.push_back(node);
			OutputDebugStringW(fmt::format(L"child {} -> parent {}\n", childTag, containerTag).c_str());
			const auto& childNode = node->yogaNode;

			if (auto childHwnd = node->window) {
				SetParent(childHwnd, parentNode->window);
			}
			if (oldParent) {
				if (!isNativeControlWithSelfLayout) {
					YGNodeRemoveChild(oldParent->yogaNode.get(), childNode.get());
				}
				auto& children = oldParent->m_children;
				auto it = std::find_if(children.cbegin(), children.cend(),
					[&node](const std::weak_ptr<ShadowNode>& other) {
						return !other.expired() && node == other.lock();
					});
				children.erase(it);
			}
			if (!isNativeControlWithSelfLayout) {
				YGNodeInsertChild(m_nodes[containerTag]->yogaNode.get(), childNode.get(), index);
				index++;
			}

		}

		DoLayout();
	});
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

void PaperUIManager::AddMeasuredRootView(Win32ReactRootView* root) {
	auto rootHwnd = root->Window();
	assert(rootHwnd != nullptr);
	m_nodes.emplace(root->Tag(), std::make_shared<ShadowNode>(rootHwnd, m_yogaConfig, nullptr));
	m_rootTag = root->Tag();
	DoLayout();
}

REACT_STRUCT(ShowAlertArgs)
struct ShowAlertArgs {
	REACT_FIELD(title)
		std::string title;

	REACT_FIELD(message)
		std::string message;

	REACT_FIELD(buttonPositive)
		std::string buttonPositive;

	REACT_FIELD(buttonNegative)
		std::string buttonNegative;

	REACT_FIELD(buttonNeutral)
		std::string buttonNeutral;

	REACT_FIELD(defaultButton)
		int defaultButton;
};



REACT_MODULE(Alert)
struct Alert : public std::enable_shared_from_this<Alert> {
	REACT_INIT(Initialize)
		void Initialize(React::ReactContext const& reactContext) noexcept { m_context = reactContext; }

	REACT_METHOD(showAlert)
		void showAlert(ShowAlertArgs const& args, std::function<void(std::string)> result) noexcept;

private:
	React::ReactContext m_context;
};


void Alert::showAlert(ShowAlertArgs const& args, std::function<void(std::string)> result) noexcept {
	auto uimgr = PaperUIManager::GetFromContext(m_context.Handle());
	auto res= MessageBoxA(uimgr->RootHWND(), args.message.c_str(), args.title.c_str(), MB_OK);
	result(res == IDOK ? "positive" : "negative");
}
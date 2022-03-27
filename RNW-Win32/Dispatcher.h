#pragma once
#include <vector>
#include <winrt/Microsoft.ReactNative.h>

struct UIDispatcher
{
    void Post(winrt::Microsoft::ReactNative::ReactDispatcherCallback cb) {
        m_tasks.push_back(cb);
    }

    void RunAll() {
        auto tasks = std::move(m_tasks);
        for (auto task : tasks)
            task();
    }

private:
    std::vector<winrt::Microsoft::ReactNative::ReactDispatcherCallback> m_tasks;
};

UIDispatcher g_uiDispatcher;

// Super simplistic dispatcher manually pumped in main loop
struct MockDispatcher : winrt::implements<MockDispatcher, winrt::Microsoft::ReactNative::IReactDispatcher> {

    MockDispatcher(UIDispatcher uiDispatcher) {
        m_threadId = std::this_thread::get_id();
    }

    bool HasThreadAccess() { return m_threadId == std::this_thread::get_id(); }
    void Post(winrt::Microsoft::ReactNative::ReactDispatcherCallback cb) {
        g_uiDispatcher.Post(cb);
    }

private:
    std::thread::id m_threadId;
};

// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// Borrowed from https://github.com/microsoft/react-native-windows/blob/main/vnext/Desktop/Modules/TimingModule.cpp

#include "pch.h"

#undef Check
using namespace std;

#include <cassert>

#include <chrono>
#include <memory>
#include <vector>

#include <NativeModules.h>

namespace facebook
{
  namespace react
  {

    using DateTime = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>;
    using TimeSpan = std::chrono::milliseconds;

    // Timer struct which holds timer id, target time, period between target time
    // and scheduling time and whether it needs to be repeated. Example:
    //           std::chrono::time_point targetTime =
    //           std::chrono::system_clock::now()+100ms; Timer timer = {12345,
    //           targetTime, 100ms, false};
    struct Timer {
      uint64_t Id;
      DateTime DueTime;
      TimeSpan Period;
      bool Repeat;
    };


    // This class is a implementation of max heap to store Timer objects.
    // The front timer has the smallest due time.
    // Example:
    //           TimerQueue tq;
    //           tq.Push(Timer{1234, now()+100ms, 100ms, false});
    //           tq.Push(Timer{1235, now()+20ms, 20ms, false});
    //           tq.Push(Timer{1236, now()+50ms, 50ms, false});
    //           tq.Pop(); //pops timer id: 1235
    //           printf("%u", tq.Front().Id); // print 1236
    class TimerQueue {
    public:
      void Push(Timer timer);
      void Pop();
      Timer& Front();
      const Timer& Front() const;
      bool Remove(uint64_t id);
      bool IsEmpty() const;

    private:
      // This vector is maintained as a max heap, where the front Timer has the
      // smallest due time.
      std::vector<Timer> m_timerVector;
    };

    // Helper class which implements createTimer, deleteTimer and setSendIdleEvents
    // for actual TimingModule Example:
    //           Timing timing;
    //           timing.createTimer(instance, id, duration, jsScheduleTime, repeat);
    //           timing.delete(id);
    REACT_MODULE(Timing)
      struct Timing : public std::enable_shared_from_this<Timing> {
      public:
        Timing() {}
        ~Timing();

        REACT_INIT(Initialize)
          void Initialize(winrt::Microsoft::ReactNative::ReactContext const& reactContext) noexcept;

        REACT_METHOD(createTimer)
          void createTimer(
            uint64_t id,
            double duration,
            double jsSchedulingTime,
            bool repeat) noexcept;
        REACT_METHOD(deleteTimer)
          void deleteTimer(uint64_t id) noexcept;
        REACT_METHOD(setSendIdleEvents)
          void setSendIdleEvents(bool sendIdleEvents) noexcept;

      private:
        static VOID CALLBACK
          ThreadpoolTimerCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_TIMER Timer) noexcept;
        void OnTimerRaised() noexcept;
        void SetKernelTimer(DateTime dueTime) noexcept;
        void InitializeKernelTimer() noexcept;
        void TimersChanged() noexcept;
        void StopKernelTimer() noexcept;
        bool KernelTimerIsAboutToFire() noexcept;
        TimerQueue m_timerQueue;
        PTP_TIMER m_threadpoolTimer = NULL;
        DateTime m_dueTime;

        winrt::Microsoft::ReactNative::ReactContext m_context;
    };



    bool operator<(const Timer& leftTimer, const Timer& rightTimer) {
      return rightTimer.DueTime < leftTimer.DueTime;
    }

    bool operator==(const Timer& leftTimer, const uint64_t id) {
      return id == leftTimer.Id;
    }

    void TimerQueue::Push(Timer timer) {
      m_timerVector.push_back(timer);
      std::push_heap(m_timerVector.begin(), m_timerVector.end());
    }

    void TimerQueue::Pop() {
      std::pop_heap(m_timerVector.begin(), m_timerVector.end());
      m_timerVector.pop_back();
    }

    Timer& TimerQueue::Front() {
      return m_timerVector.front();
    }

    const Timer& TimerQueue::Front() const {
      return m_timerVector.front();
    }

    bool TimerQueue::Remove(uint64_t id) {
      // TODO: This is very inefficient, but doing this with a heap is inherently
      // hard. If performance is not good
      //	enough for the scenarios then a different structure is probably needed.
      auto found = std::find(m_timerVector.begin(), m_timerVector.end(), id);
      if (found != m_timerVector.end()) {
        m_timerVector.erase(found);
      }
      else {
        return false;
      }
      std::make_heap(m_timerVector.begin(), m_timerVector.end());
      return true;
    }

    bool TimerQueue::IsEmpty() const {
      return m_timerVector.empty();
    }

    void Timing::Initialize(winrt::Microsoft::ReactNative::ReactContext const& context) noexcept {
      m_context = context;
    }

    /*static*/ void Timing::ThreadpoolTimerCallback(PTP_CALLBACK_INSTANCE, PVOID Parameter, PTP_TIMER) noexcept {
      static_cast<Timing*>(Parameter)->OnTimerRaised();
    }

    void Timing::OnTimerRaised() noexcept {
      m_context.JSDispatcher().Post([weakThis = std::weak_ptr<Timing>(shared_from_this())]() {
        auto strongThis = weakThis.lock();
        if (!strongThis) {
          return;
        }

        if ((!strongThis->m_threadpoolTimer) || strongThis->m_timerQueue.IsEmpty()) {
          return;
        }

        std::vector< uint64_t> readyTimers;
        auto now = std::chrono::system_clock::now();
        auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);

        // Fire timers which will be expired in 10ms
        while (!strongThis->m_timerQueue.IsEmpty() && now_ms > strongThis->m_timerQueue.Front().DueTime - 10ms) {
          // Pop first timer from the queue and add it to list of timers ready
          // to fire
          auto next = strongThis->m_timerQueue.Front();
          strongThis->m_timerQueue.Pop();

          readyTimers.push_back(next.Id);

          // If timer is repeating push it back onto the queue for the next
          // repetition 'next.Period' being greater than 10ms is intended to
          // prevent infinite loops
          if (next.Repeat)
            strongThis->m_timerQueue.Push(Timer { next.Id, now_ms + next.Period, next.Period, true });
        }

        if (!readyTimers.empty()) {
          strongThis->m_context.CallJSFunction(L"JSTimers", L"callTimers", [readyTimers = std::move(readyTimers)](winrt::Microsoft::ReactNative::IJSValueWriter const& writer) {
            writer.WriteArrayBegin();
            writer.WriteArrayBegin();
            for (auto id : readyTimers) {
              WriteValue(writer, id);
            }
            writer.WriteArrayEnd();
            writer.WriteArrayEnd();
            });
        }

        if (!strongThis->m_timerQueue.IsEmpty()) {
          strongThis->SetKernelTimer(strongThis->m_timerQueue.Front().DueTime);
        }
        else {
          strongThis->m_dueTime = DateTime::max();
        }
      });
    }

    void Timing::createTimer(
      uint64_t id,
      double duration,
      double jsSchedulingTime,
      bool repeat) noexcept {
      auto now = std::chrono::system_clock::now();
      auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);

      // Convert double duration to std::chrono::duration
      auto period = TimeSpan { (int64_t) duration };
      // Convert int64_t scheduletime to std::chrono::time_point
      DateTime scheduledTime = DateTime(TimeSpan((int64_t) jsSchedulingTime));
      // Calculate the initial due time -- scheduleTime plus duration
      auto initialDueTime = scheduledTime + period;

      if (scheduledTime + period <= now_ms && !repeat) {
        m_context.CallJSFunction(L"JSTimers", L"callTimers", [id](winrt::Microsoft::ReactNative::IJSValueWriter const& writer) {
          writer.WriteArrayBegin();
          writer.WriteArrayBegin();
          WriteValue(writer, id);
          writer.WriteArrayEnd();
          writer.WriteArrayEnd();
          });
        return;
      }

      // Make sure duration is always larger than 16ms to avoid unnecessary wakeups.
      period = TimeSpan { duration < 16 ? 16 : (int64_t) duration };
      m_timerQueue.Push(Timer { id, initialDueTime, period, repeat });

      TimersChanged();
    }

    void Timing::TimersChanged() noexcept {
      if (m_timerQueue.IsEmpty()) {
        // TimerQueue is empty.
        // Stop the kernel timer only when it is about to fire
        if (KernelTimerIsAboutToFire()) {
          StopKernelTimer();
        }
        return;
      }
      // If front timer has the same target time as ThreadpoolTimer,
      // we will keep ThreadpoolTimer unchanged.
      if (m_timerQueue.Front().DueTime == m_dueTime) {
        // do nothing
      }
      // If current front timer's due time is earlier than current
      // ThreadpoolTimer's, we need to reset the ThreadpoolTimer to current front
      // timer
      else if (m_timerQueue.Front().DueTime < m_dueTime) {
        SetKernelTimer(m_timerQueue.Front().DueTime);
      }
      // If current front timer's due time is later than current kernel timer's,
      // we will reset kernel timer only when it is about to fire
      else if (KernelTimerIsAboutToFire()) {
        SetKernelTimer(m_timerQueue.Front().DueTime);
      }
    }

    bool Timing::KernelTimerIsAboutToFire() noexcept {
      // Here we assume if kernel timer is going to fire within 2 frames (about
      // 33ms), we return true I am not sure the 2 frames assumption is good enough.
      // We may need adjustment after performance analysis.
      auto now = std::chrono::system_clock::now();
      auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
      if (m_dueTime - now_ms <= 33ms)
        return true;
      return false;
    }

    void Timing::SetKernelTimer(DateTime dueTime) noexcept {
      m_dueTime = dueTime;
      FILETIME FileDueTime;
      ULARGE_INTEGER ulDueTime;
      auto now = std::chrono::system_clock::now();
      auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
      TimeSpan period = dueTime - now_ms;
      ulDueTime.QuadPart = (ULONGLONG) -(period.count() * 10000);
      FileDueTime.dwHighDateTime = ulDueTime.HighPart;
      FileDueTime.dwLowDateTime = ulDueTime.LowPart;

      if (!m_threadpoolTimer) {
        InitializeKernelTimer();
      }

      SetThreadpoolTimer(m_threadpoolTimer, &FileDueTime, 0, 0);
    }

    void Timing::InitializeKernelTimer() noexcept {
      // Create ThreadPoolTimer
      m_threadpoolTimer = CreateThreadpoolTimer(&Timing::ThreadpoolTimerCallback, static_cast<PVOID>(this), NULL);
      assert(m_threadpoolTimer && "CreateThreadpoolTimer failed.");
    }

    void Timing::deleteTimer(uint64_t id) noexcept {
      if (m_timerQueue.IsEmpty())
        return;
      if (m_timerQueue.Remove(id)) {
        TimersChanged();
      }
    }

    void Timing::StopKernelTimer() noexcept {
      // Cancel pending callbacks
      SetThreadpoolTimer(m_threadpoolTimer, NULL, 0, 0);
      m_dueTime = DateTime::max();
    }

    void Timing::setSendIdleEvents(bool /*sendIdleEvents*/) noexcept {
      // It seems we don't need this API. Leave it empty for now.
      assert(false && "not implemented");
    }

    Timing::~Timing() {
      if (m_threadpoolTimer) {
        StopKernelTimer();
        WaitForThreadpoolTimerCallbacks(m_threadpoolTimer, true);
        CloseThreadpoolTimer(m_threadpoolTimer);
      }
    }
  } // namespace react
} // namespace facebook

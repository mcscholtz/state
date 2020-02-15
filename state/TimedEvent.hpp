#pragma once
#include "EventQueue.hpp"
#include <chrono>
#include <thread>

namespace state
{

template <typename EVENT_T>
class TimedEvent
{
public:
    TimedEvent(EventQueue<EVENT_T>& queue) : m_queue(queue){}
    
    void Schedule(int32_t ms, EVENT_T event)
    {
        auto callback = [&](int32_t ms, EVENT_T e)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            m_queue.Send(e);
        };
        std::thread t(callback, ms, event);
        t.detach();
    };

private:
    TimedEvent() = delete;
    EventQueue<EVENT_T>& m_queue;
};

}
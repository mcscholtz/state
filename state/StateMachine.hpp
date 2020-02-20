#pragma once
#include <mutex>
#include <array>
#include <atomic>

#include "EventQueue.hpp"
#include "TimedEvent.hpp"

namespace state
{

template <typename STATE_T, size_t STATE_NUM, typename EVENT_T, size_t EVENT_MAP_SIZE>
class StateMachine
{
public:
    constexpr StateMachine()
    : m_eventQueue()
    , m_timedEvent(m_eventQueue)
    , m_runEventLoop(true)
    , m_eventLoop()
    {}

    ~StateMachine()
    {
        m_runEventLoop.store(false);
        m_eventLoop.join();
    }

    virtual void ProcessEvent(EVENT_T event) = 0;

    void FutureEvent(int32_t ms, EVENT_T event)
    {
        m_timedEvent.Schedule(ms, event);
    }

protected:
    EventQueue<EVENT_T> m_eventQueue;
    TimedEvent<EVENT_T> m_timedEvent;
    std::atomic<bool>   m_runEventLoop;
    STATE_T             m_currentState;
    std::thread         m_eventLoop;

    void StartEventLoop_()
    {
        m_eventLoop = std::thread(&StateMachine::EventLoop_, this);
    }

    /* Given the current state and the occured event, 
       check if there exists a transition in the state table,
       if there is one execute the callback and return the next state */
    constexpr STATE_T OnEvent_(STATE_T current, EVENT_T event, 
        const std::array<std::tuple<STATE_T, EVENT_T, void(*)(void*),STATE_T>,EVENT_MAP_SIZE>& onEventTable)
    {
        STATE_T next = current;
        for (auto state : onEventTable)
        {
            //check if there is an entry in the transition map
            if (std::get<0>(state) == current && std::get<1>(state) == event)
            {
                //execute transition function if there is one
                auto callback = std::get<2>(state);
                
                if (callback)
                {
                    callback(this);
                }
                //get next state
                next = std::get<3>(state);
                break;
            }
        }
        return next;
    }

    constexpr void OnTransition_(STATE_T current, const std::array<std::tuple<STATE_T, void(*)(void*)>, STATE_NUM>& transitionTable)
    {
        for (auto transition : transitionTable)
        {
            if (std::get<0>(transition) == current)
            {
                auto callback = std::get<1>(transition);
                if (callback)
                {
                    callback(this);
                }
                break;
            }
        }
        return;
    }

private:

    void EventLoop_()
    {
        while(m_runEventLoop.load())
        {
            std::optional<EVENT_T> e = m_eventQueue.TryRecieve(1000);
            if (std::nullopt != e)
            {
                ProcessEvent(e.value());
            }
        }
    }
}; 

}

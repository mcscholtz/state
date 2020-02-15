#pragma once
#include <mutex>
#include <array>
#include <atomic>

#include "EventQueue.hpp"
#include "TimedEvent.hpp"

namespace state
{

template <typename STATE_T, typename EVENT_T, size_t SIZE>
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

    void StartEventLoop()
    {
        m_eventLoop = std::thread(&StateMachine::EventLoop_, this);
    }

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

    /* Given the current state and the occured event, check if there exists a transition in the state table */
    constexpr STATE_T Next(STATE_T current, EVENT_T event, 
        const std::array<std::tuple<STATE_T, EVENT_T, void(*)(void*),STATE_T>, SIZE> stateTransitionTable)
    {
        STATE_T next = current;
        for (auto state : stateTransitionTable)
        {
            if (std::get<0>(state) == current &&
                std::get<1>(state) == event)
            {
                //execute transition function
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

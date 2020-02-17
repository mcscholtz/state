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

    void ProcessEvent(EVENT_T event)
    {
        STATE_T next = OnEvent_(m_currentState, event);
        OnExit_(m_currentState);
        m_currentState = next;
        OnEnter_(m_currentState);
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

    void StartEventLoop_()
    {
        m_eventLoop = std::thread(&StateMachine::EventLoop_, this);
    }

    virtual constexpr std::array<std::tuple<STATE_T, EVENT_T, void(*)(void*),STATE_T>,EVENT_MAP_SIZE> ON_EVENT_TABLE_() = 0;

    virtual constexpr std::array<std::tuple<STATE_T, void(*)(void*)>, STATE_NUM> ON_ENTER_TABLE_() = 0;

    virtual constexpr std::array<std::tuple<STATE_T, void(*)(void*)>, STATE_NUM> ON_EXIT_TABLE_() = 0;

private:

    /* Given the current state and the occured event, 
       check if there exists a transition in the state table,
       if there is one execute the callback and return the next state */
    constexpr STATE_T OnEvent_(STATE_T current, EVENT_T event)
    {
        STATE_T next = current;
        auto onEventTable = ON_EVENT_TABLE_();
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

    //Execute the on exit callback, if there is one
    constexpr void OnExit_(STATE_T current)
    {
        //execute the onexit callback
        auto onExitTable = ON_EXIT_TABLE_();
        for (auto state : onExitTable)
        {
            if (std::get<0>(state) == current)
            {
                //execute OnExit() function if there is one defined
                auto onExit = std::get<1>(state);
                
                if (onExit)
                {
                    onExit(this);
                }
                break;
            }
        }
        return;
    }

    //Execute the on enter callback
    constexpr void OnEnter_(STATE_T current)
    {
        //execute the onexit callback
        auto onEnterTable = ON_ENTER_TABLE_();
        for (auto state : onEnterTable)
        {
            if (std::get<0>(state) == current)
            {
                //execute OnEnter() function if there is one defined
                auto onEnter = std::get<1>(state);
                
                if (onEnter)
                {
                    onEnter(this);
                }
                break;
            }
        }
        return;
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

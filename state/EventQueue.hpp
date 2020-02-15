#pragma once
#include <queue>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <optional>

namespace state
{

template <typename EVENT_T>
class EventQueue
{
    public:
        EVENT_T Recieve()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [&]{return m_queue.size() > 0;});
            EVENT_T e = m_queue.front();
            m_queue.pop();
            return e;
        }

        std::optional<EVENT_T> TryRecieve(int32_t ms)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if(!m_cv.wait_for(lock, std::chrono::milliseconds(ms), [&]{return m_queue.size() > 0;}))
            {
                return std::nullopt;
            }
            EVENT_T e = m_queue.front();
            m_queue.pop();
            
            return e;
        }

        void Send(EVENT_T e)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(e);
            m_cv.notify_one();
        }
    private:
        std::queue<EVENT_T> m_queue;
        std::mutex m_mutex;
        std::condition_variable m_cv;
};

}
#include <array>
#include <tuple>
#include <thread>
#include <iostream>
#include "../enum_utils/enum_utils/enum.hpp"
#include "../state/StateMachine.hpp"

using namespace state;
using namespace enum_utils;

enum class State
{
    Disabled = 0,
    Arming,
    Armed,
    GracePeriod,
    Triggered,
    //This has to be at the end
    NUMBER_OF_STATES
};

constexpr std::array<std::pair<State, const char *>, (size_t)State::NUMBER_OF_STATES> StateStringMap =
{
    {
        {State::Disabled,       "Disabled"},
        {State::Armed,          "Armed"},
        {State::Triggered,      "Triggered"},
        {State::Arming,         "Arming"},
        {State::GracePeriod,    "GracePeriod"},
    }
};

enum class Event
{
    EnableAlarm = 0,
    DisableAlarm,
    AlarmEnabled,
    DoorOpen,
    GracePeriodTimeout,
    //This has to be at the end
    NUMBER_OF_EVENTS
};

constexpr std::array<std::pair<Event, const char *>, (size_t)Event::NUMBER_OF_EVENTS> EventStringMap =
{
    {
        {Event::EnableAlarm,           "EnableAlarm"},
        {Event::DisableAlarm,          "DisableAlarm"},
        {Event::AlarmEnabled,          "AlarmEnabled"},
        {Event::DoorOpen,              "DoorOpen"},
        {Event::GracePeriodTimeout,    "GracePeriodTimeout"},
    }
};


class Alarm : public StateMachine<State, (size_t)State::NUMBER_OF_STATES, Event, 7>
{

public:
    Alarm()
    {
        m_currentState = State::Disabled;
        std::cout << "Initial state: " << EnumToString(m_currentState, StateStringMap) << std::endl;
        StartEventLoop_();
    }

    virtual void ProcessEvent(Event event) override
    {
        //process the incoming event
        State next = OnEvent_(m_currentState, event, ON_EVENT_TABLE);
        if (m_currentState == next)
        {
            return;
        }
        //if the state changed execute the transition functions
        OnTransition_(m_currentState, ON_EXIT_TABLE);
        m_currentState = next;
        OnTransition_(m_currentState, ON_ENTER_TABLE);
    }

private:
    static void StartGracePeriod(void * shared)
    {
        Alarm* alarm = static_cast<Alarm*>(shared);
        alarm->FutureEvent(10000, Event::GracePeriodTimeout);
        std::cout << "Starting Grace Period\n";
    }

    static void DisableAlarm(void * shared)
    {
        (void)shared;
        std::cout << "Alarm has been disabled\n";
    }

    static void AlarmArmed(void * shared)
    {
        (void)shared;
        std::cout << "The alarm is now armed\n";
    }

    static void TriggerAlarm(void * shared)
    {
        Alarm* alarm = static_cast<Alarm*>(shared);
        std::cout << "The alarm is now activated\n";
    }

    static void ArmingAlarm(void * shared)
    {
        Alarm* alarm = static_cast<Alarm*>(shared);
        alarm->FutureEvent(3000, Event::AlarmEnabled);
        std::cout << "Start arming the alarm\n";
    }

    //executed right before the state changes to the next state
    static void OnExit(void * shared)
    {
        Alarm* alarm = static_cast<Alarm*>(shared);
        std::cout << "Exit state: " << EnumToString(alarm->m_currentState, StateStringMap) << std::endl;
    }

    //executed right after the state changes to the current state
    static void OnEnter(void * shared)
    {
        Alarm* alarm = static_cast<Alarm*>(shared);
        std::cout << "Enter state: " << EnumToString(alarm->m_currentState, StateStringMap) << std::endl;
    }

    /* This table describes what happens when a specific event happens in a specific state 
     * The OnEnter and OnExit functions are still executed independantly                        */
    static constexpr std::array<std::tuple<State, Event, void(*)(void*), State>, 7> ON_EVENT_TABLE =
    {
        {
            //In this state         When this event happens       Execute this             Then go to this state
            {State::Disabled,       Event::EnableAlarm,           &ArmingAlarm,            State::Arming},
            {State::Arming,         Event::AlarmEnabled,          &AlarmArmed,             State::Armed},
            {State::Armed,          Event::DisableAlarm,          &DisableAlarm,           State::Disabled},
            {State::Armed,          Event::DoorOpen,              &StartGracePeriod,       State::GracePeriod},
            {State::GracePeriod,    Event::GracePeriodTimeout,    &TriggerAlarm,           State::Triggered},
            {State::GracePeriod,    Event::DisableAlarm,          &DisableAlarm,           State::Disabled},
            {State::Triggered,      Event::DisableAlarm,          &DisableAlarm,           State::Disabled}
        }
    };

    static constexpr std::array<std::tuple<State, void(*)(void*)>, (size_t)State::NUMBER_OF_STATES> ON_ENTER_TABLE =
    {
        {
            //When entering this state      Execute this
            {State::Disabled,               &OnEnter},
            {State::Arming,                 &OnEnter},
            {State::Armed,                  &OnEnter},
            {State::GracePeriod,            &OnEnter},
            {State::Triggered,              &OnEnter}
        }
    };

    static constexpr std::array<std::tuple<State, void(*)(void*)>, (size_t)State::NUMBER_OF_STATES> ON_EXIT_TABLE =
    {
        {
            //When exiting this state       Execute this
            {State::Disabled,               &OnExit},
            {State::Arming,                 &OnExit},
            {State::Armed,                  &OnExit},
            {State::GracePeriod,            &OnExit},
            {State::Triggered,              &OnExit}
        }
    };
}; 

int main()
{
    Alarm alarm;
    alarm.ProcessEvent(Event::EnableAlarm);
    alarm.FutureEvent(20000, Event::DoorOpen);

    while(1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}
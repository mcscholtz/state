#pragma once
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


class Alarm : public StateMachine<State, Event, 7>
{

public:
    Alarm()
    {
        m_currentState = State::Disabled;
        std::cout << "Initial state: " << EnumToString(m_currentState, StateStringMap) << std::endl;
        StartEventLoop();
    }

    void ProcessEvent(Event event) override
    {
        std::cout << "Got event: " << EnumToString(event, EventStringMap) << std::endl;
        State next = Next(m_currentState, event, m_stateTable);
        m_currentState = next;
    }

private:
    static void StartGracePeriod(void * shared)
    {
        Alarm* alarm = static_cast<Alarm*>(shared);
        //do nothing for now
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


    static constexpr std::array<std::tuple<State, Event, void(*)(void*), State>, 7> m_stateTable =
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
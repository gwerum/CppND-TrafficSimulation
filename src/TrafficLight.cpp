#include <iostream>
#include <random>
#include <future>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function.

    // Lock message queue before pulling return message from message queue
    std::unique_lock<std::mutex> uLock(_mutex);
    _condition.wait(uLock, [this] { return !_queue.empty(); }); // pass unique lock to condition variable

    // Remove last message element from queue
    T msg = std::move(_queue.back());
    _queue.pop_back();

    return msg; // will not be copied due to return value optimization (RVO) in C++
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    // Lock message queue while adding new message to queue
    std::lock_guard<std::mutex> uLock(_mutex);

    // Add message to queue
    _queue.push_back(std::move(msg));
    _condition.notify_one(); // notify client after pushing new Vehicle into vector
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    // Set initial traffic light phase
    _currentPhase = TrafficLightPhase::red;
    // Initialize message queue for traffic light phases on heap
    _currentPhaseMessageQueue = std::make_shared< MessageQueue<TrafficLightPhase> >();
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true)
    {
        TrafficLightPhase currentPhase = _currentPhaseMessageQueue->receive();
        if (currentPhase == TrafficLightPhase::green)
            return;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when 
    // the public method „simulate“ is called. To do this, use the thread queue in the base class.

    // launch processing of traffic light phase in a thread
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.

    // Initialze engine for generating random integers between 4000 and 6000 milliseconds
    std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_int_distribution<int> uni(4000, 6000);

    int cycle_duration_ms = uni(rng);
    auto last_update = std::chrono::system_clock::now();

    // Coninuously check if traffic light phase shall be switched from red to green or vice versa
    while (true)
    {
        // Compute time since last update
        long milliseconds_since_last_update = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - last_update).count();
        // "Slow down" infinite loop to prevent CPU overload
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (milliseconds_since_last_update > cycle_duration_ms)
        {
            // Update current phase (every 4 to 6 seconds)
            _currentPhase = _getNextPhaseAfter[_currentPhase];
            // Send update message and wait for it to be added to message queue
            TrafficLightPhase currentPhaseMessage = _currentPhase;
            std::future<void> ftr = std::async(
                std::launch::async, 
                &MessageQueue<TrafficLightPhase>::send, 
                _currentPhaseMessageQueue, 
                std::move(currentPhaseMessage) );
            ftr.wait();

            // Generate (random) cycle duration until next update and reset stop watch
            int cycle_duration_ms = uni(rng);
            auto last_update = std::chrono::system_clock::now();
        }
    }
}

// The following map is used to "toggle" the traffic light phase between green and red
std::map<TrafficLightPhase, TrafficLightPhase> TrafficLight::_getNextPhaseAfter = {
    {TrafficLightPhase::red, TrafficLightPhase::green },
    {TrafficLightPhase::green, TrafficLightPhase::red },
};



/** @file eventrouter.cpp
 *  @brief Thread-safe control of WAWT Screens.
 *
 * Copyright 2018 Bruce Szablak
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "wawt/eventrouter.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <utility>

#include <iostream>

namespace Wawt {

namespace {

}  // unnamed namespace

                        //-----------------------------------
                        // class Wawt::EventRouter::FifoMutex
                        //-----------------------------------

void
EventRouter::FifoMutex::lock()
{
    unsigned int myTicket = d_nextTicket++;

    std::unique_lock<std::mutex> guard(d_lock);
    d_signal.wait(guard, [this, myTicket] { return myTicket==d_nowServing; });
}

bool
EventRouter::FifoMutex::try_lock()
{
    if (d_lock.try_lock()) {
        d_nextTicket++; // since unlock will increase now serving count
        return true;
    }
    return false;
}

void
EventRouter::FifoMutex::unlock()
{
    d_nowServing += 1;
    d_signal.notify_all();
    std::this_thread::yield();
}

                            //------------------------
                            // class Wawt::EventRouter
                            //------------------------

// PRIVATE CLASS MEMBERS
EventUpCb
EventRouter::wrap(EventUpCb&& unwrapped)
{
    return  [me      = this,
             current = d_current,
             cb      = std::move(unwrapped)](int x, int y, bool up) {
                std::unique_lock<FifoMutex> guard(me->d_lock);
                assert(current == me->d_current);
                cb(x, y, up);
                me->d_downEventActive = false;
                return;
             };                                                       // RETURN
}

// PRIVATE METHODS
EventRouter::Handle // d_lock assumed to be held
EventRouter::install(Screen *screen, std::size_t hashCode)
{
    Handle handle{d_installed.size(), hashCode};
    d_installed.emplace_back(std::unique_ptr<Screen>(screen));
    return handle;                                                    // RETURN
}

// PUBLIC MEMBERS
EventRouter::EventRouter()
: d_setTimedEvent(  [this](std::chrono::milliseconds interval,
                           std::function<void()>&&   callback) {
                        d_timedCallback  = std::move(callback);
                        d_nextTimedEvent = std::chrono::steady_clock::now()
                                                + interval;
                    })
{
    d_shutdownFlag = false;
    d_spinLock.clear();
}

EventRouter::~EventRouter()
{
}

EventUpCb
EventRouter::downEvent(int x, int y)
{
    std::unique_lock<FifoMutex> guard(d_lock);

    while (d_spinLock.test_and_set());
    auto alert_p = d_alert;
    d_spinLock.clear();

    auto eventUp = EventUpCb();

    if (alert_p) {
        eventUp = alert_p->downEvent(x, y);
    }
    else if (d_current) {
        eventUp = d_current->downEvent(x, y);
    }

    if (eventUp) {
        d_downEventActive = true;
        return wrap(std::move(eventUp));                              // RETURN
    }
    return EventUpCb();                                               // RETURN
}
    
void
EventRouter::draw()
{
    std::unique_lock<FifoMutex> guard(d_lock);
    d_drawRequested = false;

    while (d_spinLock.test_and_set());
    auto alert_p = d_alert; // noexcept
    d_spinLock.clear();

    if (alert_p) {
        alert_p->draw();
        return;                                                       // RETURN
    }

    // Screen changes do not happen between a 'downEvent' and a 'EventUpCb'.

    if (!d_downEventActive) {
        while (d_spinLock.test_and_set());
        auto deferredActivate_p = std::move(d_deferredFn); // noexcept
        d_spinLock.clear();

        if (deferredActivate_p) {
            if (d_current) { // clear dialogs and timer (if anY)
                d_current->dropModalDialogBox();
                d_timedCallback  = std::function<void()>();
            }
            d_current = deferredActivate_p->first;
            d_current->synchronizeTextView();
            d_current->resize(d_currentWidth, d_currentHeight);
            deferredActivate_p->second();
        }
    }

    if (d_current) {
        d_current->draw();
    }
    return;                                                           // RETURN
}

bool
EventRouter::inputEvent(Char_t input)
{
    std::unique_lock<FifoMutex> guard(d_lock);

    while (d_spinLock.test_and_set());
    auto alert_p = d_alert;
    d_spinLock.clear();

    if (!alert_p && d_current) {
        return d_current->inputEvent(input);
    }
    
    if (d_current) {
        d_current->clearFocus();
    }
    return false;                                                     // RETURN
}

void
EventRouter::resize(double width, double height)
{
    std::unique_lock<FifoMutex> guard(d_lock);

    d_currentWidth  = width;
    d_currentHeight = height;

    while (d_spinLock.test_and_set());
    auto alert_p = d_alert; // noexcept
    d_spinLock.clear();

    if (alert_p) {
        alert_p->resizeScreen(width, height);
        return;                                                       // RETURN
    }

    if (d_current) {
        d_current->resize(width, height);
    }
    return;                                                           // RETURN
}

void
EventRouter::showAlert(const Widget&    panel,
                       double           width,
                       double           height,
                       double           percentBorder)
{
    if (width <= 1.0 && height <= 1.0 && width >  0.1 && height >  0.1) {
        auto clone  = panel.clone().layout(Layout::centered(width, height)
                                                .border(percentBorder));

        auto screen = Widget(WawtEnv::sScreen,
                             Layout()).addChild(std::move(clone));
        screen.assignWidgetIds();
        screen.synchronizeTextView(true);
        screen.resizeScreen(d_currentWidth, d_currentHeight);

        while (d_spinLock.test_and_set());
        d_alert = std::make_shared<Widget>(std::move(screen)); // noexcept
        d_spinLock.clear();
    }
    return;                                                           // RETURN
}

bool
EventRouter::tick(std::chrono::milliseconds minimumTickInterval)
{
    d_lock.lock();
    auto earliest    = d_lastTick + minimumTickInterval;
    auto calledEvent = d_drawRequested;
    d_drawRequested  = false;

    while (d_timedCallback && d_nextTimedEvent < earliest) {
        auto when = d_nextTimedEvent;
        auto now  = std::chrono::steady_clock::now();

        if (now < when) {
            d_lock.unlock();
            std::this_thread::sleep_until(when); // careful, event could change
            d_lock.lock();
        }
        else {
            auto callback = d_timedCallback;
            d_timedCallback = std::function<void()>();
            callback(); // permitted to set a new timed event.
            calledEvent = true;
        }
    }
    d_lock.unlock();
    std::this_thread::sleep_until(earliest);
    d_lastTick = std::chrono::steady_clock::now();
    return calledEvent;                                               // RETURN
}

}  // namespace Wawt
// vim: ts=4:sw=4:et:ai

/** @file wawteventrouter.cpp
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

#include "wawteventrouter.h"

#include <atomic>
#include <mutex>
#include <thread>

#include <iostream>

namespace BDS {

namespace {

}  // unnamed namespace

                        //---------------------------------
                        // class WawtEventRouter::FifoMutex
                        //---------------------------------

void
WawtEventRouter::FifoMutex::lock()
{
    unsigned int myTicket = d_nextTicket++;

    std::unique_lock<std::mutex> guard(d_lock);
    d_signal.wait(guard, [this, myTicket] { return myTicket==d_nowServing; });
}

bool
WawtEventRouter::FifoMutex::try_lock()
{
    if (d_lock.try_lock()) {
        d_nextTicket++; // since unlock will increase now serving count
        return true;
    }
    return false;
}

void
WawtEventRouter::FifoMutex::unlock()
{
    d_nowServing += 1;
    d_signal.notify_all();
    std::this_thread::yield();
}

                            //----------------------
                            // class WawtEventRouter
                            //----------------------

// PRIVATE CLASS MEMBERS
Wawt::FocusCb
WawtEventRouter::wrap(Wawt::FocusCb&& unwrapped)
{
    return  [me      = this,
             current = d_current,
             cb      = std::move(unwrapped)](Wawt::Char_t key) {
                std::unique_lock<FifoMutex> guard(me->d_lock);
                bool ret = false;

                if (current == me->d_current) {
                    ret = cb(key);
                }
                return ret;
             };                                                       // RETURN
}

Wawt::EventUpCb
WawtEventRouter::wrap(Wawt::EventUpCb&& unwrapped)
{
    return  [me      = this,
             current = d_current,
             cb      = std::move(unwrapped)](int x, int y, bool up) {
                std::unique_lock<FifoMutex> guard(me->d_lock);
                Wawt::FocusCb focusCb;

                assert(current == me->d_current);
                focusCb = cb(x, y, up);

                if (focusCb) {
                    focusCb = me->wrap(std::move(focusCb));
                }
                me->d_downEventActive = false;
                return focusCb;
             };                                                       // RETURN
}

// PRIVATE METHODS
WawtEventRouter::Handle // d_lock assumed to be held
WawtEventRouter::install(WawtScreen *screen, std::size_t hashCode)
{
    Handle handle{d_installed.size(), hashCode};
    d_installed.emplace_back(std::unique_ptr<WawtScreen>(screen));
    return handle;                                                    // RETURN
}

// PUBLIC MEMBERS
WawtEventRouter::WawtEventRouter(Wawt::DrawAdapter                 *adapter,
                                 const Wawt::TextMapper&            textMapper,
                                 const Wawt::WidgetOptionDefaults&  defaults)
: d_wawt(textMapper, adapter)
, d_setTimedEvent(  [this](std::chrono::milliseconds interval,
                           std::function<void()>&&   callback) {
                        d_timedCallback  = std::move(callback);
                        d_nextTimedEvent = std::chrono::steady_clock::now()
                                                + interval;
                    })
{
    d_wawt.setWidgetOptionDefaults(defaults);
    d_deferredFn = nullptr;
    d_alert      = nullptr;
}

Wawt::EventUpCb
WawtEventRouter::downEvent(int x, int y)
{
    std::unique_lock<FifoMutex> guard(d_lock);

    Wawt::EventUpCb eventUp;

    if (d_current) {
        eventUp = d_current->downEvent(x, y);

        if (eventUp) {
            d_downEventActive = true;
            eventUp = wrap(std::move(eventUp));
        }
    }
    return eventUp;                                                   // RETURN
}
    
void
WawtEventRouter::draw()
{
    std::unique_lock<FifoMutex> guard(d_lock);
    // Screen changes do not happen between a 'downEvent' and a 'EventUpCb'.

    if (!d_downEventActive) {
        auto deferredActivate_p
            = std::unique_ptr<DeferFn>(d_deferredFn.exchange(nullptr));

        if (deferredActivate_p) {
            (*deferredActivate_p)();
        }
    }

    if (d_current) {
        d_current->draw();
    }

    auto alert_p = d_alert.load();

    if (alert_p) {
        d_wawt.draw(*alert_p);
    }
    return;                                                           // RETURN
}

void
WawtEventRouter::resize(double width, double height)
{
    std::unique_lock<FifoMutex> guard(d_lock);

    d_currentWidth  = width;
    d_currentHeight = height;

    if (d_current) {
        d_current->resize(width, height);
    }
    return;                                                           // RETURN
}

void
WawtEventRouter::showAlert(Wawt::Panel      panel,
                           double           width,
                           double           height,
                           int              borderThickness)
{
    if (width <= 1.0 && height <= 1.0 && width >  0.1 && height >  0.1) {
        if (!panel.drawView().options().has_value()) {
            panel.drawView().options() = d_wawt.getWidgetOptionDefaults()
                                               .d_screenOptions;
        }
        panel.layoutView()
            = Wawt::Layout::centered(width, height).border(borderThickness);
        auto ptr = std::make_unique<Wawt::Panel>(std::move(panel));
        delete d_alert.exchange(ptr.release());
    }
    return;                                                           // RETURN
}

bool
WawtEventRouter::tick(std::chrono::milliseconds minimumTickInterval)
{
    d_lock.lock();
    auto calledEvent = false;
    auto earliest    = d_lastTick + minimumTickInterval;

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

}  // namespace BDS
// vim: ts=4:sw=4:et:ai

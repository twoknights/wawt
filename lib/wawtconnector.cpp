/** @file wawtconnector.cpp
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

#include "wawtconnector.h"

#include <atomic>
#include <mutex>
#include <thread>

#include <iostream>

namespace BDS {

namespace {

}  // unnamed namespace

                        //--------------------------------
                        // class WawtConnector::FifoMutex
                        //--------------------------------

void
WawtConnector::FifoMutex::lock()
{
    unsigned int myTicket = d_nextTicket++;

    std::unique_lock<std::mutex> guard(d_lock);
    d_signal.wait(guard, [this, myTicket] { return myTicket==d_nowServing; });
}

void
WawtConnector::FifoMutex::unlock()
{
    d_nowServing += 1;
    d_signal.notify_all();
    std::this_thread::yield();
}

                            //---------------------
                            // class WawtConnector
                            //---------------------

// PRIVATE MEMBERS

void
WawtConnector::forwardToController(int id, const std::any& message)
{
    // This method should only be called from a screen event handler,
    // and, therefore, the lock must already be held.
    assert(d_controller);
    if (d_asyncController) {
        std::thread callCb( [this, id, copy = message]() {
                                d_controller(id, copy);
                            });
        callCb.detach();
    }
    else {
        d_controller(id, message);
    }
    return;                                                           // RETURN
}

Wawt::FocusCb
WawtConnector::wrap(Wawt::FocusCb&& unwrapped)
{
    return  [me      = this,
             count   = d_loadCount,
             cb      = std::move(unwrapped)](Wawt::Char_t key) {
                std::unique_lock<FifoMutex> guard(me->d_lock);

                if (count == me->d_loadCount) {
                    return cb(key);
                }
                return false;
             };                                                       // RETURN
}

Wawt::EventUpCb
WawtConnector::wrap(Wawt::EventUpCb&& unwrapped)
{
    return  [me      = this,
             count   = d_loadCount,
             cb      = std::move(unwrapped)](int x, int y, bool up) {
                std::unique_lock<FifoMutex> guard(me->d_lock);
                Wawt::FocusCb focusCb;

                if (count == me->d_loadCount) {
                    focusCb = cb(x, y, up);

                    if (focusCb) {
                        focusCb = me->wrap(std::move(focusCb));
                    }
                }
                return focusCb;
             };                                                       // RETURN
}

// PUBLIC MEMBERS
WawtConnector::WawtConnector(Wawt::DrawAdapter                  *adapter,
                             const Wawt::TextMapper&            textMapper,
                             int                                screenWidth,
                             int                                screenHeight,
                             const Wawt::WidgetOptionDefaults&  defaults)
: d_lock()
, d_pending()
, d_current()
, d_wawt(textMapper, adapter)
, d_loadCount(0u)
, d_screenWidth(screenWidth)
, d_screenHeight(screenHeight)
, d_asyncController(false)
, d_controller()
{
    d_wawt.setWidgetOptionDefaults(defaults);
}

Wawt::EventUpCb
WawtConnector::downEvent(int x, int y)
{
    std::unique_lock<FifoMutex> guard(d_lock);
    auto hold = d_pending.load();

    if (hold && hold != d_current) {
        d_current = hold;
        ++d_loadCount;
    }
    Wawt::EventUpCb eventUp;

    if (d_current) {
        eventUp = d_current->downEvent(x, y);

        if (eventUp) {
            eventUp = wrap(std::move(eventUp));
        }
    }
    return eventUp;                                                   // RETURN
}
    
void
WawtConnector::draw()
{
    std::unique_lock<FifoMutex> guard(d_lock);
    auto hold = d_pending.load();

    if (hold && hold != d_current) {
        d_current = hold;
        ++d_loadCount;
    }

    if (d_current) {
        d_current->draw();
    }
    return;                                                           // RETURN
}

void
WawtConnector::resize(int width, int height)
{
    std::unique_lock<FifoMutex> guard(d_lock);
    WawtScreen *hold;

    do {
        hold = d_pending.load();

        if (hold && hold != d_current) {
            d_current = hold;
            ++d_loadCount;
        }

        if (d_current) {
            d_current->resize(width, height);
        }
    } while (hold != d_pending.load());
    return;                                                           // RETURN
}

void
WawtConnector::shutdownRequested(const std::function<void()>& completion)
{
    std::unique_lock<FifoMutex> guard(d_lock);
    auto hold = d_pending.load();

    if (hold && hold != d_current) {
        d_current = hold;
        ++d_loadCount;
    }

    if (d_current) {
        d_current->close(completion);
    }
    else {
        completion();
    }
    return;                                                           // RETURN
}

}  // namespace BDS
// vim: ts=4:sw=4:et:ai

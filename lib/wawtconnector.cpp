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
                        // class WawtConnector::FairMutex
                        //--------------------------------

void
WawtConnector::FairMutex::lock()
{
    unsigned int myTicket = d_nextTicket++;

    std::unique_lock<std::mutex> guard(d_lock);
    d_signal.wait(guard, [this, myTicket] { return myTicket==d_nowServing; });
}

void
WawtConnector::FairMutex::unlock()
{
    d_nowServing += 1;
    d_signal.notify_all();
    std::this_thread::yield();
}

                            //---------------------
                            // class WawtConnector
                            //---------------------

// PRIVATE MANIPULATORS
Wawt::FocusCb
WawtConnector::wrap(Wawt::FocusCb&& unwrapped)
{
    return  [me      = this,
             count   = d_loadCount,
             cb      = std::move(unwrapped)](wchar_t key) {
                std::unique_lock<FairMutex> guard(me->d_lock);

                if (count == me->d_loadCount) {
                    return cb(key);
                }
                return false;
             };
}

Wawt::EventUpCb
WawtConnector::wrap(Wawt::EventUpCb&& unwrapped)
{
    return  [me      = this,
             count   = d_loadCount,
             cb      = std::move(unwrapped)](int x, int y, bool up) {
                std::unique_lock<FairMutex> guard(me->d_lock);
                Wawt::FocusCb focusCb;

                if (count == me->d_loadCount) {
                    focusCb = cb(x, y, up);

                    if (focusCb) {
                        focusCb = me->wrap(std::move(focusCb));
                    }
                }
                return focusCb;
             };
}

// PUBLIC MANIPULATORS
Wawt::EventUpCb
WawtConnector::downEvent(int x, int y)
{
    std::unique_lock<FairMutex> guard(d_lock);
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
    return eventUp;
}
    
void
WawtConnector::draw()
{
    std::unique_lock<FairMutex> guard(d_lock);
    auto hold = d_pending.load();

    if (hold && hold != d_current) {
        d_current = hold;
        ++d_loadCount;
    }

    if (d_current) {
        d_current->draw();
    }
}

void
WawtConnector::resize(int width, int height)
{
    std::unique_lock<FairMutex> guard(d_lock);
    WawtScreen *hold;

    do {
        hold = d_pending.load();

        if (hold && hold != d_current) {
            d_current = hold;
            ++d_loadCount;
        }

        if (d_current) {
            std::cout << "resize: " << width << " " << height << std::endl;
            d_current->resize(width, height);
        }
    } while (hold != d_pending.load());
}

void
WawtConnector::shutdownRequested(const std::function<void()>& completion)
{
    std::unique_lock<FairMutex> guard(d_lock);
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
}

}  // namespace BDS
// vim: ts=4:sw=4:et:ai

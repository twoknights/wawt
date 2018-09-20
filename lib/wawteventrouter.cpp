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
             count   = d_activations,
             cb      = std::move(unwrapped)](Wawt::Char_t key) {
                std::unique_lock<FifoMutex> guard(me->d_lock);

                if (count == me->d_activations) {
                    return cb(key);
                }
                return false;
             };                                                       // RETURN
}

Wawt::EventUpCb
WawtEventRouter::wrap(Wawt::EventUpCb&& unwrapped)
{
    return  [me      = this,
             count   = d_activations,
             cb      = std::move(unwrapped)](int x, int y, bool up) {
                std::unique_lock<FifoMutex> guard(me->d_lock);
                Wawt::FocusCb focusCb;

                if (count == me->d_activations) {
                    focusCb = cb(x, y, up);

                    if (focusCb) {
                        focusCb = me->wrap(std::move(focusCb));
                    }
                }
                return focusCb;
             };                                                       // RETURN
}

// PRIVATE METHODS
void
WawtEventRouter::doDeferred() // d_lock assumed to be held
{
    std::lock_guard<std::mutex> guard(d_defer);
    if (d_activateFn) {
        d_activateFn();
        d_activateFn = std::function<void()>();
    }
    return;                                                           // RETURN
}

WawtEventRouter::Handle // d_lock assumed to be held
WawtEventRouter::install(std::unique_ptr<WawtScreen>&&  screen,
                         std::string_view               name,
                         std::size_t                    hashCode)
{
    Handle handle{d_installed.size(), hashCode};
    d_installed.emplace_back(std::move(screen));
    d_installed.back()->wawtScreenSetup(&d_wawt, name);
    return handle;                                                    // RETURN
}

// PUBLIC MEMBERS
WawtEventRouter::WawtEventRouter(Wawt::DrawAdapter                 *adapter,
                                 const Wawt::TextMapper&            textMapper,
                                 const Wawt::WidgetOptionDefaults&  defaults)
: d_wawt(textMapper, adapter)
{
    d_wawt.setWidgetOptionDefaults(defaults);
    d_showAlert = false;
}

Wawt::EventUpCb
WawtEventRouter::downEvent(int x, int y)
{
    std::unique_lock<FifoMutex> guard(d_lock);
    Wawt::EventUpCb eventUp;

    if (d_current) {
        eventUp = d_current->downEvent(x, y);

        if (eventUp) {
            eventUp = wrap(std::move(eventUp));
        }
    }
    doDeferred();
    return eventUp;                                                   // RETURN
}
    
void
WawtEventRouter::draw()
{
    std::unique_lock<FifoMutex> guard(d_lock);
    if (d_current) {
        d_current->draw();
    }

    if (d_showAlert.load()) {
        d_wawt.draw(d_alert);
    }
    doDeferred();
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
    doDeferred();
    return;                                                           // RETURN
}

void
WawtEventRouter::showAlert(double width, double height, Wawt::Panel panel)
{
    std::unique_lock<FifoMutex> guard(d_lock);

    if (width <= 1.0 && height <= 1.0 && width >  0.1 && height >  0.1) {
        if (!panel.drawView().options().has_value()) {
            panel.drawView().options() = d_wawt->getWidgetOptionDefaults()
                                               .d_screenOptions;
        }
        panel.layout()  = Wawt::Layout::centered(width, height);
        d_alert         = std::move(panel);
        d_showAlert     = true;
    }
    doDeferred();
    return;                                                           // RETURN
}

}  // namespace BDS
// vim: ts=4:sw=4:et:ai

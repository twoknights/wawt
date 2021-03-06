/** @file wawtconnector.h
 *  @brief Connect WAWT framework to other components.
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

#ifndef BDS_WAWTCONNECTOR_H
#define BDS_WAWTCONNECTOR_H

#include "wawt.h"
#include "wawtscreen.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>

namespace BDS {

class WawtConnector {
    // PRIVATE TYPES
    struct FairMutex {
        std::mutex                  d_lock;
        std::condition_variable     d_signal;
        std::atomic_uint            d_nextTicket;
        std::atomic_uint            d_nowServing;

        FairMutex() { d_nowServing = 0; d_nextTicket = 0; }

        void lock();
        void unlock();
    };
    // PRIVATE CLASS MEMBERS
    Wawt::FocusCb   wrap(Wawt::FocusCb&& unwrapped);

    Wawt::EventUpCb wrap(Wawt::EventUpCb&& unwrapped);

    // PRIVATE DATA MEMBERS
    FairMutex                 d_lock;
    std::atomic<WawtScreen*>  d_pending;
    WawtScreen               *d_current;
    Wawt                      d_wawt;
    unsigned int              d_loadCount;
    int                       d_screenWidth;
    int                       d_screenHeight;

  public:
    // PUBLIC CONSTRUCTORS
    WawtConnector(Wawt::DrawAdapter                  *adapter,
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
        , d_screenHeight(screenHeight) {
            d_wawt.setWidgetOptionDefaults(defaults);
        }

    WawtConnector(Wawt::DrawAdapter                  *adapter,
                   int                                screenWidth,
                   int                                screenHeight,
                   const Wawt::WidgetOptionDefaults&  defaults)
        : WawtConnector(adapter,
                         Wawt::TextMapper(),
                         screenWidth,
                         screenHeight,
                         defaults) { }

    // PUBLIC MANIPULATORS
    Wawt::EventUpCb downEvent(int x, int y);
    
    void draw();

    template<typename Func, typename... Args>
    decltype(auto) call(Func&& func, Args&&... args) {
        std::unique_lock<FairMutex> guard(d_lock);
        decltype(auto) ret{std::invoke(std::forward<Func>(func),
                                       std::forward<Args>(args)...)};
        return ret;
    }

    void resize(int width, int height);

    template <class Screen, typename... Args>
    void setupScreen(Screen                          *screen,
                     std::string_view                 name,
                     Args&&...                        args) {
        screen->wawtScreenSetup(&d_wawt, name);
        screen->setup(d_screenWidth,
                      d_screenHeight,
                      std::forward<Args>(args)...);
    }

    template <class Screen, typename... Args>
    void setCurrentScreen(Screen                     *screen,
                          Args&&...                   args) {
        try {
            screen->resetWidgets(std::forward<Args>(args)...);
        }
        catch (Wawt::Exception caught) {
            throw Wawt::Exception("Reset of screen '" + screen->name()
                                                      + "', "
                                                      + caught.what());// THROW
        }
        d_pending = screen; // handled in draw, resize, etc.
    }

    void shutdownRequested(const std::function<void()>& completion);
};

} // end BDS namespace

#endif
// vim: ts=4:sw=4:et:ai

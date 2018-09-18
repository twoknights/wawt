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
  public:
    // PUBLIC TYPES
    template <typename IF, typename... Args>
    using RetType = typename std::invoke_result_t<IF, Args...>;

  private:
    // PRIVATE TYPES
    struct FifoMutex {
        std::mutex                  d_lock;
        std::condition_variable     d_signal;
        std::atomic_uint            d_nextTicket;
        std::atomic_uint            d_nowServing;

        FifoMutex() { d_nowServing = 0; d_nextTicket = 0; }

        void lock();
        void unlock();
    };

    template <typename IF, typename... Args>
    using IsVoid = typename std::is_void<RetType<IF,Args...>>::type;

    // PRIVATE CLASS MEMBERS
    Wawt::FocusCb   wrap(Wawt::FocusCb&& unwrapped);

    Wawt::EventUpCb wrap(Wawt::EventUpCb&& unwrapped);

    // PRIVATE MANIPULATORS
    template <typename IF, typename... Args>
    RetType<IF,Args...> callProcedure(std::false_type,
                                      IF&&             func,
                                      Args&&...        args) {
        std::unique_mutex<FifoMutex> d_guard(d_lock);
        return std::invoke(std::forward<IF>(func),
                           std::forward<Args>(args)...);
    }

    template <typename IF, typename... Args>
    void callProcedure(std::true_type, IF&& func, Args&&... args) {
        std::unique_mutex<FifoMutex> d_guard(d_lock);
        std::invoke(std::forward<IF>(func), std::forward<Args>(args)...);
    }

    // PRIVATE DATA MEMBERS
    FifoMutex                 d_lock;
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
                   const Wawt::WidgetOptionDefaults&  defaults);

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
    template <typename IF, typename... Args>
    RetType<IF,Args...> callProcedure(IF&& func, Args&&... args) {
        return callProcedure(IsVoid<IF,Args...>{},
                             std::forward<IF>(func),
                             std::forward<Args>(args...));
    }
    Wawt::EventUpCb downEvent(int x, int y);
    
    void draw();

    template <class Request>
    void installController(const std::function<void(const Request&)>& callback,
                           bool                      async = false) {
        d_controller      = [callback](const std::any& request) {
                                callback(std::any_cast<Request>(request));
                            };
        d_asyncController = async;
    }

    void resize(int width, int height);

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

    template <class Screen, typename... Args>
    void setupScreen(Screen                          *screen,
                     std::string_view                 screenName,
                     int                              controllerId,
                     Args&&...                        args) {
        screen->wawtScreenSetup(&d_wawt,
                                screenName,
                                [this, controllerId](const std::any& msg) {
                                    forwardToController(controllerId, msg);
                                });
        screen->setup(d_screenWidth,
                      d_screenHeight,
                      std::forward<Args>(args)...);
    }

    void shutdownRequested(const std::function<void()>& completion);
};

} // end BDS namespace

#endif
// vim: ts=4:sw=4:et:ai

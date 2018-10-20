/** @file eventrouter.h
 *  @brief Inter-connect Wawt framework components.
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

#ifndef WAWT_EVENTROUTER_H
#define WAWT_EVENTROUTER_H

#include "screen.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace Wawt {

class EventRouter {
  public:
    // PUBLIC TYPES
    using Time = std::chrono::time_point<std::chrono::steady_clock>;

    class Handle {
        friend class EventRouter;
        std::size_t     d_index;
        std::size_t     d_hash;

        Handle(std::size_t index, std::size_t hash)
            : d_index(index), d_hash(hash) { }

      public:
        Handle() : d_index(0), d_hash(0) { }
    };

  private:
    // PRIVATE TYPES
    class FifoMutex {
        std::mutex                  d_lock;
        std::condition_variable     d_signal;
        std::atomic_uint            d_nextTicket;
        std::atomic_uint            d_nowServing;

      public:
        FifoMutex() { d_nowServing = 0; d_nextTicket = 0; }

        void lock();
        void unlock();
        bool try_lock();
    };
    using ScreenVec  = std::vector<std::unique_ptr<Screen>>;
    using DeferFn    = std::pair<Screen*,std::function<void()>>;
    using SetTimerFn = Screen::SetTimerCb;

    // PRIVATE CLASS MEMBERS
    FocusCb   wrap(FocusCb&& unwrapped);

    EventUpCb wrap(EventUpCb&& unwrapped);

    // PRIVATE MANIPULATORS
    template <class Ret, class MemFn, typename Scrn, typename... Args>
    decltype(auto) call(std::false_type,
                        MemFn           func,
                        Scrn         *screen,
                        Args&&...       args) {
        std::optional<Ret> ret;
        if (screen == d_current) {
            ret = std::invoke(func, screen, std::forward<Args>(args)...);
        }
        return ret;
    }

    template <class Ret, class MemFn, typename Scrn, typename... Args>
    decltype(auto) call(std::true_type,
                        MemFn           func,
                        Scrn         *screen,
                        Args&&...       args)  {
        std::optional<bool> ret;               
        if (screen == d_current) {             
            std::invoke(func, screen, std::forward<Args>(args)...);
            ret = true;
        }
        return ret;
    }

    Handle install(Screen *screen, std::size_t hashCode);

    template<class Scrn>
    auto resolve(const Handle& handle) {
        assert(handle.d_hash == typeid(Scrn).hash_code());
        return static_cast<Scrn*>(d_installed.at(handle.d_index).get());
    }

    // PRIVATE DATA MEMBERS
    FifoMutex                    d_lock{};
    ScreenVec                    d_installed{};
    std::function<void()>        d_timedCallback{};
    Time                         d_lastTick{};
    Time                         d_nextTimedEvent{};
    DrawProtocol                *d_adapter         = nullptr;
    Screen                      *d_current         = nullptr;
    double                       d_currentWidth    = 1280.0;
    double                       d_currentHeight   =  720.0;
    bool                         d_downEventActive = false;
    bool                         d_drawRequested   = false;
    std::unique_ptr<DeferFn>     d_deferredFn{};
    std::shared_ptr<Widget>      d_alert{};
    std::atomic_flag             d_spinLock;
    std::atomic_bool             d_shutdownFlag;
    SetTimerFn                   d_setTimedEvent;

  public:
    // PUBLIC CONSTRUCTORS
    EventRouter();

    // PUBLIC DESTRUCTORS
    ~EventRouter();

    // PUBLIC MANIPULATORS
    template<class Scrn, typename... Args>
    void activate(Handle screen, Args&&... args);

    template <typename R, class Scrn, typename... Args>
    auto call(const Handle& screen, R(Scrn::*fn), Args&&... args);

    template<class Scrn, typename... Args>
    Handle create(std::string_view name, Args&&... args);

    void discardAlert() {
        while (d_spinLock.test_and_set());
        d_alert.reset(); // noexcept
        d_spinLock.clear();
    }

    EventUpCb downEvent(int x, int y);
    
    void draw();

    bool isShuttingDown() {
        return d_shutdownFlag.load();
    }

    void resize(double width, double height);

    void showAlert(const Widget&    panel,
                   double           width           = 0.33,
                   double           height          = 0.33,
                   double           borderThickness = 2.0);

    void shuttingDown() {
        d_shutdownFlag = true;
    }

    bool tick(std::chrono::milliseconds minimumTickInterval);
};

template<class Scrn, typename... Args>
void
EventRouter::activate(Handle screen, Args&&... args)
{
    // Defer activate until next draw() event.
    using namespace std;
    Scrn *ptr = resolve<Scrn>(screen);

    auto fp = bind(&Scrn::resetWidgets, ptr, forward<Args>(args)...);
    auto un_p  = make_unique<DeferFn>(ptr, move(fp));
    
    while (d_spinLock.test_and_set());
    d_deferredFn = std::move(un_p); // noexcept
    d_spinLock.clear();
    return;                                                           // RETURN
}

template<class Scrn, typename... Args>
EventRouter::Handle
EventRouter::create(std::string_view name, Args&&... args)
{
    static_assert(std::is_convertible<Scrn,Screen>::value,
                  "'Screen' must be derived from ScreenImpl");
    auto ptr  = std::make_unique<Scrn>(std::forward<Args>(args)...);
    auto hash = typeid(*ptr).hash_code();
    ptr->wawtScreenSetup(name, d_setTimedEvent);
    ptr->setup();

    return install(static_cast<Screen*>(ptr.release()), hash);
}

template <typename R, class Scrn, typename... Args>
auto
EventRouter::call(const Handle& screen, R(Scrn::*fn), Args&&... args)
{
    std::unique_lock<FifoMutex> guard(d_lock);
    using Ret = std::invoke_result_t<decltype(fn),Scrn*,Args&&...>;

    auto p = resolve<Scrn>(screen); // Lookup pointer and cast.
    d_drawRequested = true;

    return call<Ret>(std::is_void<Ret>{}, fn, p, std::forward<Args>(args)...);
}

} // end Wawt namespace

#endif
// vim: ts=4:sw=4:et:ai

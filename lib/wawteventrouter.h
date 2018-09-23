/** @file wawteventrouter.h
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

#ifndef BDS_WAWTEVENTROUTER_H
#define BDS_WAWTEVENTROUTER_H

#include "wawt.h"
#include "wawtscreen.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace BDS {

class WawtEventRouter {
  public:
    // PUBLIC TYPES
    using Time = std::chrono::time_point<std::chrono::steady_clock>;

    class Handle {
        friend class WawtEventRouter;
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
    using ScreenVec  = std::vector<std::unique_ptr<WawtScreen>>;
    using DeferFn    = std::function<void()>;
    using SetTimerFn = WawtScreen::SetTimerCb;

    // PRIVATE CLASS MEMBERS
    Wawt::FocusCb   wrap(Wawt::FocusCb&& unwrapped);

    Wawt::EventUpCb wrap(Wawt::EventUpCb&& unwrapped);

    // PRIVATE MANIPULATORS
    template <class Ret, class MemFn, typename Screen, typename... Args>
    decltype(auto) call(std::false_type,
                        MemFn           func,
                        Screen         *screen,
                        Args&&...       args) {
        std::optional<Ret> ret;
        if (screen == d_current) {
            ret = std::invoke(func, screen, std::forward<Args>(args)...);
        }
        return ret;
    }

    template <class Ret, class MemFn, typename Screen, typename... Args>
    decltype(auto) call(std::true_type,
                        MemFn           func,
                        Screen         *screen,
                        Args&&...       args)  {
        std::optional<bool> ret;               
        if (screen == d_current) {             
            std::invoke(func, screen, std::forward<Args>(args)...);
            ret = true;
        }
        return ret;
    }

    template<class Screen, typename... Args>
    void deferredActivate(const Handle& screen, Args&&... args);

    Handle install(WawtScreen *screen, std::size_t hashCode);

    template<class Screen>
    auto resolve(const Handle& handle) {
        assert(handle.d_hash == typeid(Screen).hash_code());
        return static_cast<Screen*>(d_installed.at(handle.d_index).get());
    }

    // PRIVATE DATA MEMBERS
    FifoMutex                 d_lock{};
    std::mutex                d_defer{};
    ScreenVec                 d_installed{};
    std::function<void()>     d_timedCallback{};
    Time                      d_lastTick{};
    Time                      d_nextTimedEvent{};
    WawtScreen               *d_current         = nullptr;
    double                    d_currentWidth    = 1280.0;
    double                    d_currentHeight   =  720.0;
    bool                      d_downEventActive = false;
    bool                      d_drawRequested   = false;
    std::atomic<DeferFn*>     d_deferredFn;
    std::atomic<Wawt::Panel*> d_alert;
    std::atomic_bool          d_shutdownFlag;
    Wawt                      d_wawt;
    SetTimerFn                d_setTimedEvent;

  public:
    // PUBLIC CONSTRUCTORS
    WawtEventRouter(Wawt::DrawAdapter                 *adapter,
                    const Wawt::TextMapper&            textMapper,
                    const Wawt::WidgetOptionDefaults&  defaults);

    WawtEventRouter(Wawt::DrawAdapter                 *adapter,
                    const Wawt::WidgetOptionDefaults&  defaults)
        : WawtEventRouter(adapter, Wawt::TextMapper(), defaults) { }

    // PUBLIC MANIPULATORS
    template<class Screen, typename... Args>
    void activate(const Handle& screen, Args&&... args);

    template <typename R, class Screen, typename... Args>
    auto call(const Handle& screen, R(Screen::*fn), Args&&... args);

    template<class Screen, typename... Args>
    Handle create(std::string_view name, Args&&... args);

    void discardAlert() {
        delete d_alert.exchange(nullptr);
    }

    Wawt::EventUpCb downEvent(int x, int y);
    
    void draw();

    bool isShuttingDown() {
        return d_shutdownFlag.load();
    }

    void resize(double width, double height);

    void showAlert(Wawt::Panel      panel,
                   double           width           = 0.33,
                   double           height          = 0.33,
                   double           borderThickness = 2.0);

    void shuttingDown() {
        d_shutdownFlag = true;
    }

    bool tick(std::chrono::milliseconds minimumTickInterval);
};

template<class Screen, typename... Args>
void
WawtEventRouter::activate(const Handle& screen, Args&&... args)
{
    // Defer activate until next draw() event.
    using namespace std;
    auto mem_p = &WawtEventRouter::deferredActivate<Screen, Args...>;
    auto fp    = make_unique<DeferFn>(bind(mem_p,
                                           this,
                                           screen,
                                           forward<Args>(args)...));
    delete d_deferredFn.exchange(fp.release());
    return;                                                           // RETURN
}

template<class Screen, typename... Args>
WawtEventRouter::Handle
WawtEventRouter::create(std::string_view name, Args&&... args)
{
    static_assert(std::is_convertible<Screen,WawtScreen>::value,
                  "'Screen' must be derived from WawtScreenImpl");
    auto ptr  = std::make_unique<Screen>(std::forward<Args>(args)...);
    auto hash = typeid(*ptr).hash_code();
    ptr->wawtScreenSetup(&d_wawt, name, d_setTimedEvent);
    ptr->setup();

    return install(static_cast<WawtScreen*>(ptr.release()), hash);
}

template<class Screen, typename... Args>
void
WawtEventRouter::deferredActivate(const Handle& screen, Args&&... args)
{
    auto ptr = resolve<Screen>(screen);
    ptr->activate(d_currentWidth, d_currentHeight, args...);

    if (d_current) {
        d_current->dropModalDialogBox();
        d_timedCallback  = std::function<void()>(); // cancel timed event too
    }
    d_current = ptr;
    return;                                                           // RETURN
}

template <typename R, class Screen, typename... Args>
auto
WawtEventRouter::call(const Handle& screen, R(Screen::*fn), Args&&... args)
{
    std::unique_lock<FifoMutex> guard(d_lock);
    using Ret = std::invoke_result_t<decltype(fn),Screen*,Args&&...>;

    auto p = resolve<Screen>(screen); // Lookup pointer and cast.
    d_drawRequested = true;

    return call<Ret>(std::is_void<Ret>{}, fn, p, std::forward<Args>(args)...);
}

} // end BDS namespace

#endif
// vim: ts=4:sw=4:et:ai

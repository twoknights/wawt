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
    template <class Screen, typename MemFunc, typename... Args>
    using Ret = typename std::invoke_result_t<Screen,MemFunc,Args...>;

    class Handle {
        friend class WawtEventRouter;
        std::size_t     d_index;
        std::size_t     d_hash;
        Handle(std::size_t index, std::size_t hash);
    };

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
    using ScreenVec = std::vector<std::unique_ptr<WawtScreen>>;

    template <class Screen, typename MemF, typename... Args>
    using IsVoid = typename std::is_void<Ret<Screen,MemF,Args...>>::type;

    // PRIVATE CLASS MEMBERS
    Wawt::FocusCb   wrap(Wawt::FocusCb&& unwrapped);

    Wawt::EventUpCb wrap(Wawt::EventUpCb&& unwrapped);

    // PRIVATE MANIPULATORS
    template <class Screen, typename MemFunc, typename... Args>
    decltype(auto) call(std::false_type,
                        Screen         *screen,
                        MemFunc         func,
                        Args&&...       args) {
        std::optional<Ret<Screen,MemFunc,Args...>> ret;
        if (screen == d_current) {
            ret = std::invoke(screen, func, std::forward<Args>(args)...);
        }
        doDeferred();
        return ret;
    }

    template <class Screen, typename MemFunc, typename... Args>
    decltype(auto) call(std::false_type,
                        Screen         *screen,
                        MemFunc         func,
                        Args&&...       args) {
        std::optional<bool> ret;
        if (screen == d_current) {
            std::invoke(screen, func, std::forward<Args>(args)...);
            ret = true;
        }
        doDeferred();
        return ret;
    }

    template<class Screen, typename... Args>
    void doActivate(Handle screen, Args&& args...);

    void doDeferred();

    Handle install(std::unique_ptr<WawtScreen>&& screen,
                   std::string_view              name,
                   std::size_t                   hashCode);

    template<class Screen>
    auto resolve(const Handle& handle) {
        assert(handle.d_hash == typeid(Screen).hash_code());
        return static_cast<Screen*>(d_installed.at(handle.d_index).get());
    }

    // PRIVATE DATA MEMBERS
    FifoMutex                 d_lock{};
    std::mutex                d_defer{};
    std::function<void()>     d_activateFn{};
    ScreenVec                 d_installed{};
    Panel                     d_alert{};
    std::atomic_bool          d_showAlert{}
    WawtScreen               *d_current         = nullptr;
    double                    d_currentWidth    = 1280.0;
    double                    d_currentHeight   =  720.0;
    Wawt                      d_wawt;

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
    void activate(Handle screen, Args&& args...);

    template<class Screen, typename... Args>
    Handle create(std::string_view name, Args&& args...);

    void discardAlert() {
        d_showAlert = false;
    }

    Wawt::EventUpCb downEvent(int x, int y);
    
    void draw();

    void resize(double width, double height);

    void showAlert(double width, double height, Wawt::Panel panel);
};

template<class Screen, typename... Args>
void
WawtEventRouter::activate(const Handle& screen, Args&& args...)
{
    std::unique_lock<std::mutex> defer(d_defer);

    if (!d_lock.try_lock()) {
        d_activateFn = std::bind(WawtEventRouter::doActivate<Screen,Args...>,
                                 this,
                                 screen,
                                 std::forward<Args>(args)...);
        return;                                                       // RETURN
    }
    std::lock_guard<FifoMutex> guard(d_lock, std::adopt_lock_t{});
    defer.release();
    doActivate(screen, std::forward<Args>(args)...);
    return;                                                           // RETURN
}

template<class Screen, typename... Args>
WawtEventRouter::Handle
WawtEventRouter::create(std::string_view name, Args&& args...)
{
    static_assert(std::is_convertable<Screen,WawtScreen>,
                  "'Screen' must be derived from WawtScreenImpl");
    auto ptr      = std::make_unique<Screen>(std::forward<Args>(args)...);
    auto hash     = typeid(*ptr).hash_code();

    return install(ptr, name, hash);                                  // RETURN
}

template<class Screen, typename... Args>
void
WawtEventRouter::doActivate(const Handle& screen, Args&& args...)
{
    auto ptr = resolve<Screen>(screen);
    ptr->activate(d_currentWidth, d_currentHeight, args...);
    d_current = ptr;
    return;                                                           // RETURN
}

template <class Screen, typename MemF, typename... Args>
auto
WawtEventRouter::callCurrent(const Handle& screen, MemF func, Args&&... args)
{
    std::unique_ptr<FifoMutex> guard(d_lock);

    auto p = resolve<Screen>(screen); // make current

    return call(IsVoid<Screen, MemFunc, Args...>{},
                p,
                func,
                std::forward<Args>(args...));                         // RETURN
}

} // end BDS namespace

#endif
// vim: ts=4:sw=4:et:ai

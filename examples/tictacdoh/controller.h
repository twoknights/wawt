/** @file controller.h
 *  @brief Implement the control logic for the game.
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

#ifndef TICTACDOH_CONTROLLER_H
#define TICTACDOH_CONTROLLER_H

#include <wawt/wawt.h>
#include <wawt/eventrouter.h>

#include "stringid.h"
//#include "gamescreen.h"
#include "setupscreen.h"

#include <atomic>
#include <mutex>
#include <regex>
#include <thread>

                            //=================
                            // class Controller
                            //=================

class Controller : public SetupScreen::Calls/*, public GameScreen::Calls*/ {
  public:
    // PUBLIC TYPES
    using Handle = Wawt::EventRouter::Handle;

    // PUBLIC CONSTRUCTORS
    Controller(Wawt::EventRouter&     router,
               StringIdLookup        *mapper)
        : d_router(router), d_mapper(mapper) { d_cancel = false; }

    ~Controller();

    // SetupScreen::Calls Interface:
    StatusPair  accept(const Wawt::String_t& address)        override;

    void        cancel()                                     override;

    StatusPair  connect(const Wawt::String_t& address)       override;

    void        startGame(int)                               override;

    // GameScreen::Calls Interface:

    // void        showSetupScreen()                            override;

    // PUBLIC MANIPULATORS
    bool shutdown();

    void startup();

  private:
    std::mutex          d_cbLock;
    Handle              d_setupScreen;
    Handle              d_gameScreen;
    std::thread         d_gameThread;
    std::atomic_bool    d_cancel;
    Wawt::EventRouter&  d_router;
    StringIdLookup     *d_mapper;
};

#endif

// vim: ts=4:sw=4:et:ai

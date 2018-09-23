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

#include "stringid.h"
#include "gamescreen.h"
#include "setupscreen.h"

#include <SFML/Network/TcpListener.hpp>
#include <SFML/Network/TcpSocket.hpp>

#include <atomic>
#include <regex>

                            //=================
                            // class Controller
                            //=================

class Controller : public SetupScreen::Calls {
    using Handle = WawtEventRouter::Handle;

    WawtEventRouter&    d_router;
    StringIdLookup&     d_mapper;
    Handle              d_setupScreen;
    Handle              d_gameScreen;
    sf::TcpListener     d_listener{};
    sf::TcpSocket       d_connection{};
    std::regex          d_addressRegex{R"(^([a-z.\-\d]+):(\d+)$)"};
    std::atomic_bool    d_cancel;

  public:
    // PUBLIC TYPES

    // PUBLIC CONSTRUCTORS
    Controller(WawtEventRouter& router, StringIdLookup& mapper)
        : d_router(router), d_mapper(mapper) { }

    // SetupScreen::Calls Interface:
    StatusPair listen(const Wawt::String_t& port)           override;

    StatusPair connect(const Wawt::String_t& connectString) override;

    void       cancel()                                     override;

    void       showGameScreen(int  marker)                  override;

    // PUBLIC MANIPULATORS
    void accept();

    void asyncConnect(sf::IpAddress address, int port);

    bool shutdown();

    void startup();
};

#endif

// vim: ts=4:sw=4:et:ai

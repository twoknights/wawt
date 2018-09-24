/** controller.cpp
 *
 *  Establish language, network connection, and game settings.
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

#include "controller.h"

#include <SFML/Network/SocketSelector.hpp>

#include <chrono>
#include <string>
#include <thread>

using namespace std::literals::chrono_literals;

namespace {

int stoint(const Wawt::String_t& s) {
    std::size_t pos;
    auto        converted = SfmlDrawAdapter::toAnsiString(s);
    int         num = -1;
    try {
        num = std::stoi(converted, &pos);
    }
    catch (...) {
    }
    return converted.empty() || pos != converted.length() ? -1 : num;
}

} // unnamed namespace

                            //-----------------
                            // class Controller
                            //-----------------

void
Controller::accept()
{
    // A timed "wait" operation is used to poll the cancel flag while watching
    // for an indication that the listen socket has something to accept.
    sf::SocketSelector select;
    select.add(d_listener);

    d_connection.disconnect();
    d_connection.setBlocking(true);

    while (!d_cancel) {
        if (select.wait(sf::seconds(1.0f))) {
            if (select.isReady(d_listener)
             && sf::Socket::Done == d_listener.accept(d_connection)) {
                d_listener.close();
                auto callRet = d_router.call(d_setupScreen,
                                             &SetupScreen::connectionResult,
                                             true,
                                             S("Connection established."),
                                             &d_connection);

                if (!callRet.has_value()) {
                    d_connection.disconnect();
                }
                return;                                               // RETURN
            }
        }
    }
    d_router.call(d_setupScreen,
                  &SetupScreen::connectionResult,
                  false,
                  S("Connect attempt cancelled."),
                  nullptr);
    d_listener.close();
    return;                                                           // RETURN
}

void
Controller::asyncConnect(sf::IpAddress address, int port)
{
    unsigned short port_us = static_cast<unsigned short>(port);
    d_connection.setBlocking(false);

    while (!d_cancel) {
        sf::Socket::Status status = d_connection.connect(address, port_us);

        if (sf::Socket::Done == status) {
            d_connection.setBlocking(true);
            auto callRet = d_router.call(d_setupScreen,
                                         &SetupScreen::connectionResult,
                                         true,
                                         S("Connection established."),
                                         &d_connection);
            if (!callRet.has_value()) {
                d_connection.disconnect();
            }
            return;                                                   // RETURN
        }

        if (sf::Socket::NotReady != status) {
            d_connection.disconnect();
        }
        std::this_thread::sleep_for(1s);
    }
    d_connection.disconnect();

    d_router.call(d_setupScreen,
                  &SetupScreen::connectionResult,
                  false,
                  S("Connect attempt cancelled."),
                  nullptr);
    return;                                                           // RETURN
}

void
Controller::cancel()
{
    d_cancel = true;
}

Controller::StatusPair
Controller::connect(const Wawt::String_t& connectString)
{
    std::string destination = SfmlDrawAdapter::toAnsiString(connectString);
    std::size_t pos;
    std::smatch addressAndPort;
    int         port;
    // parse based on <host>:<port>:

    if (!std::regex_match(destination, addressAndPort, d_addressRegex)
     || (port = std::stoi(addressAndPort.str(2),&pos)
         , pos != addressAndPort.str(2).length())) {
        return {false, S("The destination string is malformed.")};
    }
    // This can result in a DNS lookup:
    sf::IpAddress ipV4(addressAndPort.str(1));

    if (sf::IpAddress::Any       == ipV4
     || sf::IpAddress::Broadcast == ipV4
     || port < 0
     || port > UINT16_MAX) {
        return {false, S("The destination host or port is invalid.")};
    }

    if (sf::IpAddress::None == ipV4) {
        return {false, S("The destination host is unknown.")};
    }
    d_cancel = false;

    auto thread = std::thread(  [this, ipV4, port]() {
                                    asyncConnect(ipV4, port);
                                });

    if (!thread.joinable()) {
        return {false, S("Failed to request connection.")};
    }
    thread.detach();
    return {true, S("Attempting to connect to opponent.")};           // RETURN
}

Controller::StatusPair
Controller::listen(const Wawt::String_t& portString)
{
    auto port = stoint(portString);

    if (port < 0 || port > UINT16_MAX) {
        return {false,
                S("A port number between 0 and 65535 must be used.")};
    }
    d_listener.close();
    
    if (sf::Socket::Error == d_listener.listen(port)) {
        return {false,
                S("Failed to listen on port. It may be already in use.")};
    }
    d_cancel = false;
    auto thread = std::thread([this]() { accept(); });
    
    if (!thread.joinable()) {
        return {false, S("Failed to wait for a connection.")};
    }
    thread.detach();
    return {true, S("Waiting for opponent to connect.")};             // RETURN
}

bool
Controller::shutdown()
{
    d_router.showAlert(Wawt::Panel({},
                       {
                       Wawt::Label(Wawt::Layout::slice(false, 0.1, 0.3),
                                  {S("Do you wish to exit the game?")}),
                       Wawt::ButtonBar(Wawt::Layout::slice(false, -0.3, -0.1),
                                      {
                                        {{S("Yes")},
                                         [this](auto) {
                                            d_router.discardAlert();
                                            d_router.shuttingDown();
                                            return Wawt::FocusCb();
                                         }},
                                        {{S("No")},
                                         [this](auto) {
                                            d_router.discardAlert();
                                            return Wawt::FocusCb();
                                         }}
                                      })
                       }));
    return false;                                                     // RETURN
}

void
Controller::startup()
{
    d_setupScreen = d_router.create<SetupScreen>("Setup Screen",
                                                 this,
                                                 std::ref(d_mapper));
    d_gameScreen  = d_router.create<GameScreen>("Game Screen",
                                                 this);
    /* ... additional screens here ...*/

//    d_router.activate<SetupScreen>(d_setupScreen);
    d_router.activate<GameScreen>(d_gameScreen, Wawt::String_t("X"));
    // Ready for events to be processed.
    return;                                                           // RETURN
}

void
Controller::showSetupScreen()
{
    d_router.activate<SetupScreen>(d_setupScreen);
}

void
Controller::showGameScreen(const Wawt::String_t& marker)
{
    d_router.activate<GameScreen>(d_gameScreen, marker);
}

// vim: ts=4:sw=4:et:ai

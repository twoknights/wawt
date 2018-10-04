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

} // unnamed namespace

                            //-----------------
                            // class Controller
                            //-----------------

void
Controller::cancel()
{
    std::lock_guard<std::mutex> guard(d_cbLock);
    d_ipc->closeConnection(d_currentId);
}

void
Controller::connectionChange(WawtIpcProtocol::ConnectionId     id,
                             WawtIpcProtocol::ConnectionStatus status)
{
    // Note: no other connection update for 'id' can be delivered
    // until this function returns.
    std::lock_guard<std::mutex> guard(d_cbLock);

    if (d_currentId == id) {
        if (WawtIpcProtocol::ConnectionStatus::eOK == status) {
            d_cbLock.unlock();

            auto callRet = d_router.call(d_setupScreen,
                                         &SetupScreen::connectionResult,
                                         true,
                                         S("Connection established."));
            d_cbLock.lock();

            if (!callRet) {
                // No longer interested in this connection's updates.
                d_ipc->closeConnection(id);
            }
            return;                                                   // RETURN
        }
        d_currentId = WawtIpcProtocol::kINVALID_ID;

        if (WawtIpcProtocol::ConnectionStatus::eCLOSED == status) {
            d_cbLock.unlock();
            d_router.call(d_setupScreen,
                          &SetupScreen::connectionResult,
                          false,
                          S("Connect attempt cancelled."));
            d_cbLock.lock();
        }
        // TBD: DISCONNECT
    }
    return;                                                           // RETURN
}

Controller::StatusPair
Controller::connect(const Wawt::String_t& connectString)
{
    Wawt::String_t           diagnostic;

    WawtIpcProtocol::ConnectionId id;

    auto status = d_ipc->prepareConnection(&diagnostic,
                                           &id,
                                           [this](auto cid, auto info) {
                                              connectionChange(cid, info);
                                           },
                                           [this](auto, auto) {
                                           },
                                           connectString);

    if (WawtIpcProtocol::AddressStatus::eOK != status) {
        return {false, diagnostic};
    }
    std::lock_guard<std::mutex> guard(d_cbLock);

    if (!d_ipc->startConnection(&diagnostic, id)) {
        return {false, diagnostic};
    }
    d_currentId = id;

    return {true, S("Attempting to connect to opponent.")};           // RETURN
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
                                            d_ipc->closeAll();
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

    d_router.activate<SetupScreen>(d_setupScreen);
//    d_router.activate<GameScreen>(d_gameScreen, Wawt::String_t("X"));
    // Ready for events to be processed.
    return;                                                           // RETURN
}

void
Controller::showSetupScreen()
{
    d_router.activate<SetupScreen>(d_setupScreen);
}

void
Controller::startGame(const Wawt::String_t& marker, int)
{
    d_router.activate<GameScreen>(d_gameScreen, marker);
}

// vim: ts=4:sw=4:et:ai

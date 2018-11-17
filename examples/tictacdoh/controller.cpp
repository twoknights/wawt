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

#include <chrono>
#include <mutex>
#include <string>
#include <thread>

#undef  S
#undef  C

#ifdef WAWT_WIDECHAR
#define S(str) (L"" str)  // wide char strings (std::wstring)
#define C(c) (L ## c)
#else
#define S(str) (u8"" str)      // UTF8 strings  (std::string)
#define C(c) (U ## c)
#endif

using namespace std::literals::chrono_literals;

namespace {

} // unnamed namespace

                            //-----------------
                            // class Controller
                            //-----------------

Controller::~Controller()
{
}

Controller::StatusPair
Controller::accept(const Wawt::String_t& address)
{
}

void
Controller::cancel()
{
    std::lock_guard<std::mutex> guard(d_cbLock);
    //d_ipc->closeChannel(d_currentId);
}

#if 0
void
Controller::connectionChange(Wawt::IpcProtocol::ChannelId,
                             Wawt::IpcProtocol::ChannelStatus)
{
    // Note: no other connection update for 'id' can be delivered
    // until this function returns.
    std::lock_guard<std::mutex> guard(d_cbLock);

    if (d_currentId == id) {
        if (Wawt::IpcProtocol::ChannelStatus::eOK == status) {
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
#endif

Controller::StatusPair
Controller::connect(const Wawt::String_t&)
{
#if 0
    Wawt::String_t           diagnostic;

    Wawt::IpcProtocol::ChannelId id;

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
#endif

    return {true, S("Attempting to connect to opponent.")};           // RETURN
}

bool
Controller::shutdown()
{
    using namespace Wawt;
    using namespace Wawt::literals;
    d_router.showAlert(
            panel().addChild(
                        label(Layout().scale(1.0, 0.2),
                              S("Do you wish to exit the game?")))
                   .addChild(
                        pushButtonGrid({{-1.0, 0.6}, {1.0, 0.9}}, -1.0, 1_Sz,
                                       {{S("Yes"), [this](auto) {
                                             //d_ipc->closeAll();
                                             d_router.discardAlert();
                                             // event-loop checks the router
                                             // shutdown flag (do this last):
                                             d_router.shuttingDown();
                                        }},
                                        {S("No"), [this](auto) {
                                             d_router.discardAlert();
                                         }}})));
    return false;                                                     // RETURN
}

void
Controller::startup()
{
    d_setupScreen = d_router.create<SetupScreen>("Setup Screen",
                                                 this,
                                                 d_mapper);
    //d_gameScreen  = d_router.create<GameScreen>("Game Screen", this);
    /* ... additional screens here ...*/

    d_router.activate<SetupScreen>(d_setupScreen);
    // Ready for events to be processed.
    return;                                                           // RETURN
}

#if 0
void
Controller::showSetupScreen()
{
    d_router.activate<SetupScreen>(d_setupScreen);
}
#endif

void
Controller::startGame(int)
{
    d_router.activate<SetupScreen>(d_setupScreen);
}

// vim: ts=4:sw=4:et:ai

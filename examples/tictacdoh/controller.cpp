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
Controller::establishConnection(bool                   listen,
                                const Wawt::String_t&  address,
                                const Wawt::String_t&  moveTime)
{
    auto  diagnostic = Wawt::String_t{};
    auto  completion =
        [this, moveTime] (Wawt::IpcMessage       *dropIndication,
                          Wawt::IpcMessage       *handshake,
                          const Ticket&,
                          bool                    success,
                          const Wawt::String_t&   message) -> bool {
            std::unique_lock guard(d_cbLock);
            //TBD: set ipc messages
            d_setupTicket.reset();
            return d_router.call(d_setupScreen,
                                 &SetupScreen::connectionResult,
                                 success,
                                 success ? S("Connection established.")
                                         : message).value_or(false);
        };
    d_setupTicket  = d_ipc->remoteSetup(&diagnostic,
                                        listen,
                                        address,
                                        completion);

    if (d_setupTicket) {
        if (listen) {
            diagnostic = S("Expecting connection on port: ")
                       + Wawt::toString(d_listenPort(d_setupTicket));
        }
        else {
            diagnostic = S("Attempting to connect to opponent.");
        }
    }
    return {!!d_setupTicket, diagnostic};                             // RETURN
}

void
Controller::cancel()
{
    std::lock_guard<std::mutex> guard(d_cbLock);

    if (d_setupTicket) {
        d_ipc->cancelRemoteSetup(d_setupTicket);
        d_setupTicket.reset();
    }
}

bool
Controller::shutdown()
{
    using namespace Wawt;
    using namespace Wawt::literals;
    d_router.showAlert(
            panel().options(std::any_cast<DrawOptions>(
                                    WawtEnv::defaultOptions(WawtEnv::sDialog))
                        .fillColor(DrawOptions::kGREY))
                   .addChild(
                        label(Layout().scale(1.0, 0.2).translate(0,-.2),
                              S("Do you wish to exit the game?")))
                   .addChild(
                        pushButtonGrid({{-1.0, 0.5}, {1.0, 0.9}}, -1.0, 1_Sz,
                                       {{S("Yes"), [this](auto) {
                                             d_ipc->shutdown();
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
    d_gameScreen  = d_router.create<GameScreen>("Game Screen",
                                                 this,
                                                 d_mapper);
    /* ... additional screens here ...*/

    d_router.activate<SetupScreen>(d_setupScreen);
    // Ready for events to be processed.
    return;                                                           // RETURN
}

void
Controller::showSetupScreen()
{
    d_router.activate<SetupScreen>(d_setupScreen);
}

void
Controller::startGame(int)
{
    // game screen:
    d_router.activate<GameScreen>(d_gameScreen);
}

// vim: ts=4:sw=4:et:ai

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

#include "gamescreen.h"
#include "setupscreen.h"

#include <chrono>
#include <thread>

namespace BDS {

namespace {

} // unnamed namespace

                            //-----------------
                            // class Controller
                            //-----------------

void
Controller::startup()
{
    auto doSetup = [this]() { setupScreen(); };
    d_setupScreen = d_router.create<SetupScreen>("Setup Screen", d_mapper);
    d_gameScreen  = d_router.create<GameScreen>("Game Screen", doSetup);
    /* ... additional screens here ...*/

    d_router.activate<GameScreen>(d_gameScreen);
    // Ready for events to be processed.
}

void
Controller::setupScreen()
{
    d_router.activate<SetupScreen>(d_setupScreen);
}

}  // namespace BDS

// vim: ts=4:sw=4:et:ai

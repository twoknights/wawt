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
Controller::installScreens(SetupScreen *setup)
{
    d_connector.setControllerCallback([this](int id, const std::any& request) {
        processScreenRequest(id, request);
    }, true);

    d_setupScreen = setup;
    d_connector.setupScreen(setup, "Setup Screen", eSETUPSCREEN);
    d_connector.setCurrentScreen(setup); // no optional arguments
}

void
Controller::processScreenRequest(int, const std::any&)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(5000)); // FIX THIS
    d_connector.voidCall(&SetupScreen::dropModalDialogBox, d_setupScreen);
}

}  // namespace BDS

// vim: ts=4:sw=4:et:ai

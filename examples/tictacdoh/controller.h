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

#ifndef BDS_TICTACDOH_CONTROLLER_H
#define BDS_TICTACDOH_CONTROLLER_H

#include "setupscreen.h"
#include "wawtconnector.h"

namespace BDS {

                            //=================
                            // class Controller
                            //=================

class Controller {
    WawtConnector&      d_connector;
    SetupScreen        *d_setupScreen;

  public:
    // PUBLIC TYPES
    enum ScreenId {
        eSETUPSCREEN
    };

    // PUBLIC CONSTRUCTORS
    Controller(WawtConnector& connector) : d_connector(connector) { }

    // PUBLIC MANIPULATORS
    void installScreens(SetupScreen *setup);

    void processScreenRequest(int id, const std::any& request);
};

} // end BDS namespace

#endif

// vim: ts=4:sw=4:et:ai

///////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Bruce Szablak
//
///////////////////////////////////////////////////////////////////////////////
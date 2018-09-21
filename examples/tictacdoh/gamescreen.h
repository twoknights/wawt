/** @file gamescreen.h
 *  @brief Game play screen for Tic-Tac-DOH!
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

#ifndef BDS_TICTACDOH_GAMESCREEN_H
#define BDS_TICTACDOH_GAMESCREEN_H

#include "stringid.h"
#include "drawoptions.h"

#include "wawtscreen.h"

#include <iostream>

namespace BDS {

                            //=================
                            // class GameScreen
                            //=================

//
// The game screen consists of the tic-tac-toe board.  When both players
// select the same cell, a pop-up dialog presents a selection for a
// "rock-scissors-paper" round which continues until there is a winner.
//
class GameScreen : public WawtScreenImpl<GameScreen,DrawOptions> {
    
    // PRIVATE DATA MEMBERS

  public:
    // PUBLIC TYPES

    // PUBLIC CONSTRUCTORS
    GameScreen() : WawtScreenImpl() { }

    // PUBLIC MANIPULATORS
    // Called by 'WawtScreenImpl::setup()':
    Panel createScreenPanel();

    // Called by 'WawtScreenImpl::activate()':
    void resetWidgets();

    FocusCb click(int square, Wawt::Text *button);
};


} // end BDS namespace

#endif

// vim: ts=4:sw=4:et:ai

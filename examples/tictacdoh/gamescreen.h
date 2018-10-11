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

#ifndef TICTACDOH_GAMESCREEN_H
#define TICTACDOH_GAMESCREEN_H

#include "stringid.h"

#include <iostream>

                            //=================
                            // class GameScreen
                            //=================

//
// The game screen consists of the tic-tac-toe board.  When both players
// select the same cell, a pop-up dialog presents a selection for a
// "rock-scissors-paper" round which continues until there is a winner.
//
class GameScreen : public Wawt::ScreenImpl<GameScreen,DrawOptions> {
  public:
    // PUBLIC TYPES
    struct Calls {
        virtual ~Calls() { }

        virtual void showSetupScreen()                                     = 0;
    };

    enum GameResult { eFORFEIT };

    // PUBLIC CONSTRUCTORS
    GameScreen(Calls *controller)
        : ScreenImpl(), d_controller(controller) { }

    // PUBLIC MANIPULATORS
    // Called by 'WawtScreenImpl::setup()':
    Wawt::Widget createScreenPanel();

    // Called by 'WawtScreenImpl::activate()':
    void gameOver(GameResult result);

    void resetWidgets(const Wawt::String_t& marker);

    void startGame();

    void showRemainingTime();

private:
    // PRIVATE DATA MEMBERS
    GameScreen::Calls      *d_controller;
    Wawt::Widget           *d_timeLabel;
    Wawt::Widget           *d_boardPanel;
    Wawt::String_t          d_marker;
    int                     d_countDown = -1;
};

#endif

// vim: ts=4:sw=4:et:ai

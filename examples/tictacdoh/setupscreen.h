/** @file setupscreen.h
 *  @brief Setup screen for Tic-Tac-DOH!
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

#ifndef TICTACDOH_SETUPSCREEN_H
#define TICTACDOH_SETUPSCREEN_H

#include "stringid.h"
#include "drawoptions.h"

#include <wawt/wawt.h>
#include <wawt/screen.h>

#include <utility>

                            //==================
                            // class SetupScreen
                            //==================

//
// The setup screen will have a "select list" to change the current
// language selection, a pair of "radio" buttons to select if the player
// wants their marker to be an 'X' or an 'O' (a random toss determines
// which player get's their choice honored), and a pair of text entry
// boxes which contain either the port the player wants to wait for
// a network connection, or a connect address used to try and setup a 
// network connection.  When a connection (and random toss) are complete
// the results are saved using the arguments the screen was constructed
// with, and the controller is notified using a callback defined when
// the screen was created.
//
class SetupScreen : public Wawt::ScreenImpl<SetupScreen,DrawOptions> {
  public:
    // PUBLIC TYPES

    struct Calls {
        using StatusPair = std::pair<bool, Wawt::String_t>;

        virtual ~Calls() { }

        virtual StatusPair accept(const Wawt::String_t& address)        = 0;

        virtual void       cancel()                                     = 0;

        virtual StatusPair connect(const Wawt::String_t& address)       = 0;

        virtual void       startGame(int)                               = 0;
    };

    // PUBLIC CONSTRUCTORS
    SetupScreen(Calls *controller, StringIdLookup *mapper)
        : ScreenImpl(), d_controller(controller), d_mapper(mapper) { }

    // PUBLIC MANIPULATORS
    // Called by 'WawtScreenImpl::setup()':
    Wawt::Widget createScreenPanel();

    void         initialize();

    // Called by 'WawtScreenImpl::activate()':
    void resetWidgets();

    void connectCallback(bool listen);

    void connectionResult(bool success, Wawt::String_t status);

  private:  
    // PRIVATE DATA MEMBERS
    Wawt::TextEntry         d_connectEntry{35};
    Wawt::TextEntry         d_listenPortEntry{5};
    int                     d_moveTime = 10;
    Wawt::Tracker           d_languageList;
    Calls                  *d_controller;
    StringIdLookup         *d_mapper;
};

#endif

// vim: ts=4:sw=4:et:ai

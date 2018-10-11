/** gamescreen.cpp
 *
 *  Present tic-tac-toe board.
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

#include "wawt/wawt.h"

#include "gamescreen.h"

#include <chrono>
#include <string>

using namespace std::literals::chrono_literals;

namespace {

} // unnamed namespace

using namespace Wawt;
                            //-----------------
                            // class GameScreen
                            //-----------------

Widget
GameScreen::createScreenPanel()
{
    return panel({})
      .addChild(panel(&d_boardPanel,
                      {{-1.0,-1.0},{1.0,1.0}, Vertex::eUPPER_CENTER, 0.0})
                .addChild(panelGrid(Layout::centered(0.6, 0.6).border(0), 3, 3,
                                    pushButton({{},{},5},
                                               [](auto w) {
                                                  w->resetLabel(S("X"), false);
                                               },
                                               S(" "))))
                .addChild(panel({{-1.0,-1.0,0_wr},{1.0,1.0,0_wr},5})
                          .options(DrawOptions().lineColor(
                                     defaultOptions(WawtEnv::sScreen)
                                        .d_fillColor)))
                .addChild(label({{-1.0,-1.0},{1.0,0.7}},
                                S("Tic-Tac-DOH!"))))
      .addChild(pushButton({{},{-0.95,-0.95}, Vertex::eUPPER_LEFT},
                           {[this](auto) { d_screen.serialize(std::cout); }},
                           S("*")));
}

void
GameScreen::gameOver(GameResult result)
{
    String_t resultString;

    if (result == GameResult::eFORFEIT) {
        resultString = S("You have forfeited the game.");
    }

#if 0
    addModalDialogBox(
        {
            Label(Layout::slice(false, 0.1, 0.3), {resultString, 254_F}),
            Label(Layout::slice(false, 0.35, 0.55),
                    {S("Would you like to play again?"), 254_F}),
            ButtonBar(Layout::slice(false, -0.25, -0.08),
                {
                  {{S("Play")}, [this](auto) { dropModalDialogBox();
                                               startGame();
                                               return FocusCb();}},
                  {{S("Quit")}, [this](auto) { dropModalDialogBox();
                                               d_controller->showSetupScreen();
                                               return FocusCb();}}
                })
        });
#endif
}

void
GameScreen::resetWidgets(const String_t& marker)
{
    d_marker    = marker;
    d_boardPanel->setHidden(true);
    d_boardPanel->setDisabled(true);
#if 0
    addModalDialogBox(
        {
            Label(Layout::slice(false, 0.1, 0.3),
                    {S("Click 'Ready' to begin the game."), 255_F}),
            Label(Layout::slice(false, 0.35, 0.55),
                    {S("The round time is 10s."), 255_F}),
            ButtonBar(Layout::slice(false, -0.25, -0.08),
                {
                  {S("Ready"), [this](auto) { dropModalDialogBox();
                                              startGame();
                                              return FocusCb();}}
                })
        });
#endif
}

void
GameScreen::showRemainingTime()
{
    if (d_countDown == 0) {
        gameOver(eFORFEIT);
    }
    else {
        String_t message
            = S("Remaining time: ") + Wawt::toString(d_countDown--);
        d_timeLabel->resetLabel(message, true);
        resize();
        setTimedEvent(1000ms, [this]() { showRemainingTime(); });
    }
}

void
GameScreen::startGame()
{
    d_boardPanel->setHidden(false);
    d_boardPanel->setDisabled(false);
    d_countDown = 10;
    showRemainingTime();
}

// vim: ts=4:sw=4:et:ai

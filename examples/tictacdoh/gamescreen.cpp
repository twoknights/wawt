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
    auto click = [](auto *w) { w->resetLabel("X"); };

    auto  screen     = panelGrid({}, 1, 3, panel({}));
    auto  screenFill = defaultOptions(WawtEnv::sScreen).d_fillColor;
    auto  overlayOpt = DrawOptions(DrawOptions::kCLEAR, screenFill);
    auto& middle     = screen.children()[1];
    auto  border     = 5.0;
    middle.addChild( // 0_wr
            panelGrid(&d_boardPanel,
                      {{-1,-1},{ 1, 1}, Layout::Vertex::eCENTER_CENTER},
                      3,
                      3,
                      pushButton(Layout().border(border), click, S(" "))))
          .addChild( // 1_wr
            panel({{-1,-1, 0_wr},{ 1, 1, 0_wr}, border}).options(overlayOpt))
          .addChild( // 2_wr - this is a "shim"
            panel({{-1,-1},{-1,-1, 0_wr}}))
          .addChild( // 3_wr
            label({{ 1,-1, 2_wr},{ 1,-1, 0_wr}}, S("Tic-Tac-Toe")));
    return screen;
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
#if 0
    d_boardPanel->setHidden(true);
    d_boardPanel->setDisabled(true);
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

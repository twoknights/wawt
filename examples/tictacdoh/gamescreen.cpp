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

#include "gamescreen.h"

#include <chrono>
#include <string>

using namespace std::literals::chrono_literals;

namespace {

} // unnamed namespace

                            //-----------------
                            // class GameScreen
                            //-----------------

Wawt::Panel
GameScreen::createScreenPanel()
{
    auto   mkClick = [me = this](auto sq) {
        return [me, sq](auto btn) { me->click(sq, btn); return FocusCb(); };
    };
    double third   = 1.0/3.0;
    return Panel({},
      {
      Panel(&d_boardPanel,{{-1.0,-1.0},{1.0,1.0}, Vertex::eUPPER_CENTER, 0.0},
        {
        Panel(Layout::centered(0.6, 0.6).border(0),
          {
          Button({{-1.0, -1.0}, {-third,-third}, 5},
                  mkClick(1),
                  {S(""), Align::eCENTER}),
          Button(Layout::duplicate(1_wr,5).translate(2.0,0.0),
                  mkClick(2),
                  {S(""), Align::eCENTER}),
          Button(Layout::duplicate(1_wr,5).translate(4.0,0.0),
                  mkClick(3),
                  {S(""), Align::eCENTER}),

          Button(Layout::duplicate(1_wr,5).translate(0.0,2.0),
                  mkClick(4),
                  {S(""), Align::eCENTER}),
          Button(Layout::duplicate(1_wr,5).translate(2.0,2.0),
                  mkClick(5),
                  {S(""), Align::eCENTER}),
          Button(Layout::duplicate(1_wr,5).translate(4.0,2.0),
                  mkClick(6),
                  {S(""), Align::eCENTER}),

          Button(Layout::duplicate(1_wr,5).translate(0.0,4.0),
                  mkClick(7),
                  {S(""), Align::eCENTER}),
          Button(Layout::duplicate(1_wr,5).translate(2.0,4.0),
                  mkClick(8),
                  {S(""), Align::eCENTER}),
          Button(Layout::duplicate(1_wr,5).translate(4.0,4.0),
                  mkClick(9),
                  {S(""), Align::eCENTER})
          }),
        Panel({{-1.0,-1.0,1_wr},{1.0,1.0,1_wr},5},
              DrawOptions().lineColor(defaultScreenOptions().d_fillColor)),
        Label(Layout::slice(false, 0.05, 0.15), S("Tic-Tac-DOH!")),
        Label(&d_timeLabel, Layout::slice(false,-0.17,-0.10), S(""))
        })
    , Button({{},{-0.95,-0.95}, Vertex::eUPPER_LEFT},
            {[this](auto) { d_screen.serialize(std::cout);
            return FocusCb();
            }, ActionType::eCLICK},
            {S("*")})
      });
}

void
GameScreen::gameOver(GameResult result)
{
    String_t resultString;

    if (result == GameResult::eFORFEIT) {
        resultString = S("You have forfeited the game.");
    }

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
}

void
GameScreen::resetWidgets(const String_t& marker)
{
    d_marker    = marker;
    d_boardPanel->setEnablement(Enablement::eHIDDEN);
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
}

Wawt::FocusCb
GameScreen::click(int, Wawt::Text *button)
{
    button->textView().setText(d_marker);
    return FocusCb();
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
        d_timeLabel->textView().setText(message);
        resize();
        setTimedEvent(1000ms, [this]() { showRemainingTime(); });
    }
}

void
GameScreen::startGame()
{
    d_boardPanel->setEnablement(Enablement::eACTIVE);
    d_countDown = 10;
    showRemainingTime();
}

// vim: ts=4:sw=4:et:ai

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
        Panel(Layout::centered(0.5, 0.5).border(0).pin(Vertex::eCENTER_CENTER),
        {
            Button({{-1.0, -1.0}, {-third,-third}, 5},
                   mkClick(1),
                   {S(""), Align::eCENTER}),
            Button(Wawt::Layout::duplicate(1_wr,5).translate(2.0,0.0),
                   mkClick(2),
                   {S(""), Align::eCENTER}),
            Button(Wawt::Layout::duplicate(1_wr,5).translate(4.0,0.0),
                   mkClick(3),
                   {S(""), Align::eCENTER}),

            Button(Wawt::Layout::duplicate(1_wr,5).translate(0.0,2.0),
                   mkClick(4),
                   {S(""), Align::eCENTER}),
            Button(Wawt::Layout::duplicate(1_wr,5).translate(2.0,2.0),
                   mkClick(5),
                   {S(""), Align::eCENTER}),
            Button(Wawt::Layout::duplicate(1_wr,5).translate(4.0,2.0),
                   mkClick(6),
                   {S(""), Align::eCENTER}),

            Button(Wawt::Layout::duplicate(1_wr,5).translate(0.0,4.0),
                   mkClick(7),
                   {S(""), Align::eCENTER}),
            Button(Wawt::Layout::duplicate(1_wr,5).translate(2.0,4.0),
                   mkClick(8),
                   {S(""), Align::eCENTER}),
            Button(Wawt::Layout::duplicate(1_wr,5).translate(4.0,4.0),
                   mkClick(9),
                   {S(""), Align::eCENTER})
        })
        , Button({{},{-0.95,-0.95}, Vertex::eUPPER_LEFT},
                 {[this](auto) { d_screen.serialize(std::cout);
                                 d_notify();
                                 return FocusCb();
                               }, ActionType::eCLICK},
                 {S("*")})
    });
}

void
GameScreen::resetWidgets()
{
}

Wawt::FocusCb
GameScreen::click(int square, Wawt::Text *button)
{
    char         c = '0' + square;
    Wawt::Char_t ch = static_cast<Wawt::Char_t>(c);
    button->textView().setText(Wawt::String_t(1, ch));
    return FocusCb();
}

// vim: ts=4:sw=4:et:ai

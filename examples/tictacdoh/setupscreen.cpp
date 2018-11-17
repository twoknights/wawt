/** setupscreen.cpp
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

#include "setupscreen.h"
#include "stringid.h"

#include <wawt/drawprotocol.h>

#include <chrono>
#include <string>
#include <iostream>

#ifdef WAWT_WIDECHAR
#define S(str) (L"" str)  // wide char strings (std::wstring)
#define C(c) (L ## c)
#else
#define S(str) (u8"" str)      // UTF8 strings  (std::string)
#define C(c) (U ## c)
#endif

using namespace std::literals::chrono_literals;

namespace {

} // unnamed namespace

                            //------------------
                            // class SetupScreen
                            //------------------

Wawt::Widget
SetupScreen::createScreenPanel()
{
    using namespace Wawt;
    using namespace Wawt::literals;

    auto changeLanguage = [this](auto, uint16_t row) {
        d_mapper->currentLanguage(static_cast<StringIdLookup::Language>(row));
        // Since this results in string IDs taking on new string values,
        // the framework needs to reload those values.
        synchronizeTextView();
        // Now re-layout the screen:
        resize(); // not acutally changing the size.
    };

    auto languageList =
        panel(Layout().scale(0.3, 0.4))
            .addChild(
                label({{-1.0,-1.0},{ 1.0,-0.7}}, StringId::eSelectLanguage))
            .addChild(
                fixedSizeList(d_languageList,
                              {{-1.0, 1.0, 0_wr},{ 1.0, 1.0}},
                              true,
                              changeLanguage,
                              2_Sz,
                              {
                                  S("English"),
                                  S("Deutsch"),
                                  S("Español"),
                                  S("Français"),
                                  S("Italiano"),
                                  S("Polski"),
                                  S("Pусский")
                              }));

    auto leftSideContents =
        panel({{-1.0, 1.0, 0_wr},{ 0.0, 1.0}})
            .addChild(std::move(languageList))
            .addChild(
                pushButton({{},{-0.95,-0.95}, Layout::Vertex::eUPPER_LEFT},
                           [this](auto) { d_screen.serialize(std::cout);
                                          Wawt::DrawStream draw;
                                          d_screen.draw(&draw); },
                           S("*")));

    auto networkSettings =
        panel({{-1.0, -0.9},{ 1.0, 0.0}})
            .addChild(
                concatenateLabels({{-1.0,-0.9},{ 1.0,-0.6}}, 2_Sz,
                                  TextAlign::eLEFT,
                                  {{StringId::eWaitForConnection},
                                   {&d_listenPortEntry}}))
            .addChild(
                label({{-1.0, 0.0},{ 1.0, 0.3}},
                      StringId::eConnectToOpponent, 2_Sz, TextAlign::eLEFT))
            .addChild(
                label(d_connectEntry,
                      {{-1.0, 0.3},{ 1.0, 0.6}},
                      d_connectEntry.layoutString(), 2_Sz, TextAlign::eLEFT));

    auto rightSideContents =
        panel({{ 0.0, 1.0, 0_wr},{ 0.8, 1.0}})
            .addChild(std::move(networkSettings));

    return
        panel({})
            .addChild(
                label({{-1.0,-0.9},{ 1.0,-0.7}}, StringId::eGameSettings))
            .addChild(std::move(leftSideContents))
            .addChild(std::move(rightSideContents));

#if 0
            {
/* 4(2,3) */    Panel(Layout::centered(0.5, 0.75).translate(0.,-.25),
                      selectLanguage)
            }),
            Panel(Layout::slice(true, 0.5, 0.95), // right half of screen
            {
/* 6 */         List(&d_playerMark,
                     Layout::slice(false, 0.25, 0.40),
                     2_F,
                     Wawt::ListType::eRADIOLIST,
                     {
                         { StringId::ePlayAsX, true },
                         { StringId::ePlayAsO}
                     }),
                 TextEntry({{-1.0,-0.05},{-0.9,0.05}}, 2, changeMoveTime,
                           {S("10"), 2_F, Align::eCENTER}),
                 Label({{1.5,-1.0,2_wr},{1.0,0.05}},
                       {S(": Move timer (seconds)"), 2_F, Align::eLEFT}),
                Panel(Layout::slice(false, 0.6, 0.8), networkConnect)
            })
#endif
}

void
SetupScreen::initialize()
{
    d_languageList->children()[0].selected(true);
}

void
SetupScreen::resetWidgets()
{
    d_listenPortEntry.entry(S(""));
    d_connectEntry.entry(S(""));
}

// vim: ts=4:sw=4:et:ai

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

#include <chrono>
#include <string>

using namespace std::literals::chrono_literals;

// String literals must have the appropriate prefix.
// See: Wawt::Char_t and Wawt::String_t

#undef  S
#undef  C
//#define S(str) Wawt::String_t(str)      // ANSI strings  (std::string)
//#define C(c) (c)
//#define S(str) Wawt::String_t(L"" str)  // UCS-2 strings (std::wstring)
//#define C(c) (L ## c)
#define S(str) Wawt::String_t(U"" str)  // UTF-32 strings (std::u32string)
#define C(c) (U ## c)

namespace BDS {

namespace {

} // unnamed namespace

                            //------------------
                            // class SetupScreen
                            //------------------

Wawt::Panel
SetupScreen::createScreenPanel()
{
    auto changeLanguage = [this](auto, uint16_t index) {
        d_mapper.currentLanguage(static_cast<StringIdLookup::Language>(index));
        // Since this results in string IDs taking on new string values,
        // the framework needs to reload those values.  This only happens
        // on resize:
        resize(); // not acutally changing the size.
        return Wawt::FocusCb(); // no text entry block gets the focus
    };

    auto listenPopUp =
        Panel({}, defaultScreenOptions(),
        {
            Label(Layout::slice(false, 0.1, 0.3),
                  {S("Waiting for opponent."), Align::eCENTER})
        });

    auto listenTo  = [this,listenPopUp](auto) {
        addModalDialogBox(listenPopUp, 0.33, 0.33, 2);
        setTimedEvent(5000ms, [this]() { dropModalDialogBox(); });
        return true;
    };
    auto connectTo = Wawt::EnterFn();

    Panel::Widgets selectLanguage = // 2 widgets
        {
            Label(Layout::slice(false, 0.25, 0.45),
                  {StringId::eSelectLanguage, Align::eCENTER}),
            List({{-0.8, 1., 1_wr},{0.8, 5., 1_wr}},
                 1_F,
                 Wawt::ListType::eSELECTLIST,
                 {
                     { S("English")  , true },
                     { S("Deutsch")   },
                     { S("Español")   },
                     { S("Français")  },
                     { S("Italiano")  },
                     { S("Polski")    },
                     { S("Pусский")   }
                 },
                 changeLanguage)
        };

    Panel::Widgets networkConnect = // 4 widgets
        {
            Label(Layout::slice(false, 0.0, 0.22),
                  {StringId::eWaitForConnection, 2_F, Align::eLEFT}),
            TextEntry({{-1.0,1.1, 1_wr},{-0.5,3., 1_wr}},
                      5,
                      listenTo,
                      {StringId::eNone, 3_F, Align::eLEFT}),
            Label(Layout::slice(false, -0.45, -0.23),
                  {StringId::eConnectToOpponent, 2_F, Align::eLEFT}),
            TextEntry(&d_connectEntry,
                      Layout::slice(false, -0.22, 0.0),
                      40,
                      connectTo,
                      {StringId::eNone, 3_F, Align::eLEFT})
        };

    return Panel({},
        {
/* 1 */     Label(Layout::slice(false, 0.1, 0.2),
                  {StringId::eGameSettings, Align::eCENTER}),
/* 5 */     Panel(Layout::slice(true, 0.0, 0.5),
            {
/* 4(2,3) */    Panel(Layout::centered(0.5, 0.75).translate(0.,-.25),
                      selectLanguage)
            }),
            Panel(Layout::slice(true, 0.5, 0.95),
            {
/* 6 */         List(Layout::slice(false, 0.25, 0.40),
                     2_F,
                     Wawt::ListType::eRADIOLIST,
                     {
                         { StringId::ePlayAsX, true },
                         { StringId::ePlayAsO}
                     }),
/* 11(7-10)*/   Panel(Layout::slice(false, 0.5, 0.7), networkConnect)
            })
            , Button({{},{-0.95,-0.95}, Vertex::eUPPER_LEFT},
                     {[this](auto) { d_screen.serialize(std::cout);
                                     return FocusCb();
                                   }, ActionType::eCLICK},
                     {S("*")})
        });                                                           // RETURN
}

void
SetupScreen::resetWidgets()
{
    d_connectEntry->setEnablement(Enablement::eOFF);
}

}  // namespace BDS

// vim: ts=4:sw=4:et:ai

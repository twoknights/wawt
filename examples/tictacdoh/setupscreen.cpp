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

SetupScreen::SetupScreen(Calls              *controller,
                         StringIdLookup     *mapper)
: ScreenImpl()
, d_connectEntry(35,
    [this](Wawt::TextEntry *textEntry, Wawt::Char_t) -> bool {
        return entryCallback(false, textEntry);
    })
, d_listenPortEntry(5,
    [this](Wawt::TextEntry *textEntry, Wawt::Char_t) -> bool {
        return entryCallback(true, textEntry);
    })
, d_controller(controller)
, d_mapper(mapper)
, d_moveClock(0.3, { { S("5") } , { S("10"), true } , { S("15") } })
, d_languageList({
        { S("English"),  true  }
      , { S("Deutsch")  }
      , { S("Español")  }
      , { S("Français") }
      , { S("Italiano") }
      , { S("Polski")   }
      , { S("Pусский")  }
      })
{
    d_languageList.singleSelectList(true);
}

void
SetupScreen::cancelConnect(bool success, bool listen)
{
    d_controller->cancel();

    if (listen) {
        d_listenPortEntry.entry(S(""));
    }
    else {
        d_connectEntry.entry(S(""));
    }

    if (!success) {
        dropModalDialogBox();
    }
    return;                                                           // RETURN
}

void
SetupScreen::connectionResult(bool success, const Wawt::String_t& message)
{
    using namespace Wawt;
    using namespace Wawt::literals;

    auto closeAction =
        [this, success](Widget *widget) {
            widget->popDialog();
            if (success) {
                auto timeout = d_moveClock.selectedLabel();
                d_controller->startGame(std::stoi(timeout.data()));
            }
            else {
                d_listenPortEntry.entry(S(""));
                d_connectEntry.entry(S(""));
            }
        };
    auto buttons = pushButtonGrid({}, 5.0, kNOGROUP,
                                  {{S("Close"), closeAction }});
    auto dialog = dialogBox(
            Layout().scale(0.33, 0.33),
            std::move(buttons),
            { { S("") }, {message, 1_Sz} });
    addModalDialogBox(std::move(dialog));
    return;                                                           // RETURN
}

Wawt::Widget
SetupScreen::createScreenPanel()
{
    using namespace Wawt;
    using namespace Wawt::literals;

    auto changeLanguage = [this](ScrolledList*,
                                 ScrolledList::ItemIter itemIter) {
        auto row = std::distance(d_languageList.rows().begin(), itemIter);
        d_mapper->currentLanguage(static_cast<StringIdLookup::Language>(row));
        // Since this results in string IDs taking on new string values,
        // the framework needs to reload those values.
        synchronizeTextView();
        // Now re-layout the screen:
        resize(); // not acutally changing the size.
    };
    d_languageList.onItemClick(changeLanguage);

    auto languageList =
        panel({{-0.5,-0.9},{0.5,-0.1}})
            .addChild(
                label({{-1.0,-1.0},{ 1.0,-0.7}}, StringId::eSelectLanguage))
            .addChild(
                d_languageList.widget()
                              .layout({{-0.8, 1.0, 0_wr},{ 0.8, 1.0}}));

    auto leftSideContents =
        panel({{-1.0, 1.0, 0_wr},{-0.1, 1.0}})
            .addChild(std::move(languageList))
            .addChild(
                pushButton({{},{-0.95,-0.95}, Layout::Vertex::eUPPER_LEFT},
                           [this](auto) { d_screen.serialize(std::cout);
                                          Wawt::DrawMock draw;
                                          d_screen.draw(&draw); },
                           S("*")));

    auto blankRow =
        label({}, S(""));

    auto clockSetting = 
        concatenateTextWidgets({},
                               2_Sz, TextAlign::eLEFT,
                               label(Layout(), StringId::eMoveClock,
                                     2_Sz, TextAlign::eLEFT),
                               d_moveClock.widget());

    auto listenSetting =
        concatenateTextWidgets({},
                               2_Sz, TextAlign::eLEFT,
                               label({}, StringId::eWaitForConnection),
                               d_listenPortEntry.widget());

    auto connectLabel  =
        label({}, StringId::eConnectToOpponent, 2_Sz, TextAlign::eLEFT);

    auto connectSetting = 
        d_connectEntry.widget().charSizeGroup(2_Sz)
                               .horizontalAlign(TextAlign::eLEFT);

    auto rightSideContents =
        panelLayout({{ 0.0, 1.0, 0_wr},{ 0.9, 0.0}}, 0.0, 1,
                    blankRow.clone(),
                    std::move(clockSetting),
                    blankRow.clone(),
                    std::move(listenSetting),
                    std::move(connectLabel),
                    std::move(connectSetting));

    return
        panel({})
            .addChild(
                label({{-1.0,-0.9},{ 1.0,-0.7}}, StringId::eGameSettings))
            .addChild(std::move(leftSideContents))
            .addChild(std::move(rightSideContents));                  // RETURN

}

bool
SetupScreen::entryCallback(bool listen, Wawt::TextEntry *entry)
{
    using namespace Wawt;
    using namespace Wawt::literals;

    auto address            = entry->entry();
    auto moveClock          = (*d_moveClock.selectedRow().value())->d_view;
    auto [success, message] =
        d_controller->establishConnection(listen,
                                          address,
                                          String_t(moveClock.d_viewFn()));
    auto buttons = pushButtonGrid({}, 5.0, kNOGROUP,
                                  {{ success ? S("Cancel") : S("Close"),
                                   [this, listen, success](Widget *) {
                                        cancelConnect(success, listen); }} });
    auto label1  = listen ? StringId::eWaitForConnection
                          : StringId::eConnectToOpponent;
    auto dialog = dialogBox(
            Layout().scale(0.33, 0.33),
            std::move(buttons),
            { {label1, 1_Sz},
              {address, 1_Sz},
              {S(""), 1_Sz},
              {std::move(message), 2_Sz} });
    addModalDialogBox(std::move(dialog));
    return false; // lose focus                                       // RETURN
}

void
SetupScreen::initialize()
{
    d_languageList->children()[0].selected(true);
    return;                                                           // RETURN
}

void
SetupScreen::resetWidgets()
{
    d_listenPortEntry.entry(S(""));
    d_connectEntry.entry(S(""));
    return;                                                           // RETURN
}

// vim: ts=4:sw=4:et:ai

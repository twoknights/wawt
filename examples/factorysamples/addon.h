/** @file addon.h
 *  @brief Text Entry and Variable List Samples
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

#ifndef FACTORYSAMPLES_ADDON_H
#define FACTORYSAMPLES_ADDON_H

#include <drawoptions.h>
#include <wawt/layout.h>
#include <wawt/screen.h>
#include <wawt/scrolledlist.h>
#include <wawt/textentry.h>
#include <wawt/wawtenv.h>
#include <wawt/widgetfactory.h>

#include <iostream>
#include <string>

                                //=============
                                // class Addons
                                //=============

class Addons : public Wawt::ScreenImpl<Addons,DrawOptions> {
  public:
    // PUBLIC TYPES
    struct Range {
        int              d_min{};
        int              d_max{};
        Wawt::TextEntry *d_next = nullptr;

        static bool isDigit(Wawt::TextEntry*, Wawt::Char_t input) {
            return Wawt::String_t::npos
                                != Wawt::String_t("0123456789").find(input);
        }

        bool    operator()(Wawt::TextEntry *entry, Wawt::Char_t input) {
            auto string = entry->entry();

            if (string.empty()) {
                return true;
            }
            auto value  = std::stoi(string);

            if (value < d_min || value >d_max) {
                return true; // keep focus
            }
            
            if (input == entry->enterChar() || input == '\t') {
                if (d_next) {
                    (*d_next)->focus(&**d_next);
                }
            }
            return false;
        }
    };

    // PUBLIC CONSTRUCTORS
    Addons(Wawt::OnClickCb&& prev, Wawt::OnClickCb&& next)
        : d_next(std::move(next))
        , d_prev(std::move(prev))
        , d_buttons()
        , d_list(7)
        , d_enterRow(25,
                [this](auto text, auto ch) -> bool {
                    auto string = text->entry();
                    if (ch && !string.empty()) {
                        d_list.rows().emplace_front(string, false);
                        d_list.synchronizeView();
                        text->entry(S(""));
                    }
                    return true;
                })
        , d_month(2, Range{   1,   12, &d_day  }, {'\t'})
        , d_day(  2, Range{   1,   31, &d_year }, {'\t'})
        , d_year( 4, Range{2018, 2199, &d_month}, {'\t'}) {
            d_list.onItemClick(
                [this](auto,auto) {
                    d_buttons->children()[2]
                              .disabled(d_list.selectCount() == 0);
                });
            d_month.inputVerifier(&Range::isDigit).autoEnter(true);
              d_day.inputVerifier(&Range::isDigit).autoEnter(true);
             d_year.inputVerifier(&Range::isDigit).autoEnter(true);
    }

    // PUBLIC MANIPULATORS
    // Called by 'WawtScreenImpl::setup()':
    Wawt::Widget createScreenPanel();

    // Called by 'WawtScreenImpl::activate()':
    void resetWidgets() {
        d_buttons->children()[2].disabled(true);
        d_list.clear();
        d_enterRow.entry("");
        d_month.entry("");
        d_day.entry("");
        d_year.entry("");
    }

private:
    // PRIVATE DATA MEMBERS
    Wawt::OnClickCb     d_next;
    Wawt::OnClickCb     d_prev;
    Wawt::Tracker       d_buttons;
    Wawt::ScrolledList  d_list;
    Wawt::TextEntry     d_enterRow;
    Wawt::TextEntry     d_month;
    Wawt::TextEntry     d_day;
    Wawt::TextEntry     d_year;
};

inline Wawt::Widget
Addons::createScreenPanel()
{
    using namespace Wawt;
    auto addTop =
        [this](Widget *) -> void {
            auto string = d_enterRow.entry();
            if (!string.empty()) {
                d_list.rows().emplace_front(string, false);
                d_list.synchronizeView();
                d_enterRow.entry(S(""));
                d_enterRow->focus(&*d_enterRow);
            }
        };
    auto addBot = 
        [this](Widget *) -> void {
            auto string = d_enterRow.entry();
            if (!string.empty()) {
                d_list.rows().emplace_back(string, false);
                d_list.synchronizeView();
                d_enterRow.entry(S(""));
                d_enterRow->focus(&*d_enterRow);
            }
        };
    auto delSel = 
        [this](Widget *) -> void {
        };

    auto lineColor   = defaultOptions(WawtEnv::sPanel)
                          .lineColor(defaultOptions(WawtEnv::sScreen)
                                          .d_fillColor);
    auto entryOptions = defaultOptions(WawtEnv::sEntry);
    auto titleBar = S("Text Entry and Variable Sized Lists.");
    auto screen =
        panel().addChild(
                    label({{-1.0, -1.0}, {1.0, -0.9}, 0.1}, titleBar)
                    .downEventMethod(&dumpScreen)
                    .options(defaultOptions(WawtEnv::sLabel)
                             .fillColor({235,235,255})))
               .addChild(
                    pushButtonGrid({{-1.0, 0.9}, {1.0, 1.0}}, -1.0, 1_Sz,
                                   {{d_prev,    S("Prev")},
                                    {d_next,    S("Next")}})
                                    .border(5).options(lineColor))
               .addChild(
                    concatenateLabels({{-0.7, -0.85}, {0.7, -0.75}}, 2_Sz,
                                      TextAlign::eCENTER, {
                            { S("Enter today's date: ") },
                            { &d_month, entryOptions },
                            { S("/") },
                            { &d_day, entryOptions },
                            { S("/") },
                            { &d_year, entryOptions },
                            { S(" (TextEntry & concatenated labels)") },
                        }))
               .addChild(
                    label({{-1.0,-0.65},{1.0,-0.55}},
                           S("Variable List with Scrolling")))
               .addChild(
                    d_list.list({{-0.2,-0.5},{ 0.2, 0.0}}))
               .addChild(
                    label(d_enterRow,
                          {{-0.25, 0.05}, {0.25, 0.13}},
                          d_enterRow.layoutString(),
                          3_Sz, TextAlign::eRIGHT))
               .addChild(
                    label({{ 0.27, 0.05}, {1.0, 0.13}},
                          S(" (Text to add)"), 3_Sz, TextAlign::eLEFT))
               .addChild(
                    pushButtonGrid(d_buttons,
                                   {{-0.2, 0.2}, { 0.2, .3}}, -1.0, 1_Sz,
                                   {{addTop,  S("AddTop")},
                                    {addBot,  S("AddBot")},
                                    {delSel,  S("DelSel")}}));


    return screen;                                                    // RETURN
}

#endif

// vim: ts=4:sw=4:et:ai

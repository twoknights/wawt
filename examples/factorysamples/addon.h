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
        , d_sample(25)
        , d_month(2, Range{   1,   12, &d_day  }, {'\t'})
        , d_day(  2, Range{   1,   31, &d_year }, {'\t'})
        , d_year( 4, Range{2018, 2199, &d_month}, {'\t'}) { }

    // PUBLIC MANIPULATORS
    // Called by 'WawtScreenImpl::setup()':
    Wawt::Widget createScreenPanel();

    // Called by 'WawtScreenImpl::setup()':
    void initialize() {
        using namespace Wawt;
        auto& container = lookup(4_wr);
        auto *month     = container.lookup(1_wr);

        if (!month) {
            throw WawtException("Month entry has invalid widget ID.");
        }
        auto *day       = container.lookup(3_wr);

        if (!day) {
            throw WawtException("Day entry has invalid widget ID.");
        }
        auto *year      = container.lookup(5_wr);

        if (!year) {
            throw WawtException("Year entry has invalid widget ID.");
        }
        month->changeTracker(d_month); // This updates widget's methods too.
          day->changeTracker(d_day);
         year->changeTracker(d_year);

        d_month.inputVerifier(&Range::isDigit);
          d_day.inputVerifier(&Range::isDigit);
         d_year.inputVerifier(&Range::isDigit);
    }

    // Called by 'WawtScreenImpl::activate()':
    void resetWidgets() { }

private:
    // PRIVATE DATA MEMBERS
    Wawt::OnClickCb d_next;
    Wawt::OnClickCb d_prev;
    Wawt::TextEntry d_sample;
    Wawt::TextEntry d_month;
    Wawt::TextEntry d_day;
    Wawt::TextEntry d_year;
};

inline Wawt::Widget
Addons::createScreenPanel()
{
    using namespace Wawt;
    auto lineColor   = defaultOptions(WawtEnv::sPanel)
                          .lineColor(defaultOptions(WawtEnv::sScreen)
                                          .d_fillColor);
    auto entryOptions = defaultOptions(WawtEnv::sEntry);
    auto titleBar = S("Text Entry and Variable Sized Lists.");
    auto screen =
        panel().addChild( // RID: 0
                    label({{-1.0, -1.0}, {1.0, -0.9}, 0.1}, titleBar)
                    .options(defaultOptions(WawtEnv::sLabel)
                             .fillColor({235,235,255})))
               .addChild( // RID: 1
                    pushButtonGrid({{-1.0, 0.9}, {1.0, 1.0}}, -1.0, 1_Sz,
                                   {{d_prev,    S("Prev")},
                                    {d_next,    S("Next")}})
                                    .border(5).options(lineColor))
               .addChild( // RID: 2
                    label(d_sample, {{-1.0, -0.9}, {0.0, -0.8}, 0.1},
                               d_sample.layoutString(),
                               2_Sz, TextAlign::eRIGHT))
               .addChild( // RID: 3
                    label({{ 0.0, -0.9}, {1.0, -0.8}, 0.1},
                                S("<-Click and type."), TextAlign::eLEFT))
               .addChild( // RID: 4
                    concatenateLabels({{-0.5, -0.8}, {0.5, -0.7}}, 3_Sz,
                                      TextAlign::eCENTER, {
                            { S("Enter today's date: ") },            // RID 0
                            { d_month.layoutString(), entryOptions }, // RID 1*
                            { S("/") },                               // RID 2
                            { d_day.layoutString(), entryOptions },   // RID 3*
                            { S("/") },                               // RID 4
                            { d_year.layoutString(), entryOptions }   // RID 5*
                        }));

    return screen;                                                    // RETURN
}

#endif

// vim: ts=4:sw=4:et:ai

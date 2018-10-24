/** @file button.h
 *  @brief Push buttons and grids.
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

#ifndef FACTORYSAMPLES_BUTTON_H
#define FACTORYSAMPLES_BUTTON_H

#include <drawoptions.h>
#include <wawt/layout.h>
#include <wawt/screen.h>
#include <wawt/wawtenv.h>
#include <wawt/widgetfactory.h>

#include <iostream>

                                //==============
                                // class Buttons
                                //==============

class Buttons : public Wawt::ScreenImpl<Buttons,DrawOptions> {
  public:
    // PUBLIC TYPES

    // PUBLIC CONSTRUCTORS
    Buttons(Wawt::FocusChgCb&& prev, Wawt::FocusChgCb&& next)
        : d_next(std::move(next)), d_prev(std::move(prev)) { }

    // PUBLIC MANIPULATORS
    // Called by 'WawtScreenImpl::setup()':
    Wawt::Widget createScreenPanel();

    // Called by 'WawtScreenImpl::activate()':
    void resetWidgets() {
        using namespace Wawt;
    }

private:
    // PRIVATE DATA MEMBERS
    Wawt::FocusChgCb d_next;
    Wawt::FocusChgCb d_prev;
};

inline Wawt::Widget
Buttons::createScreenPanel()
{
    using namespace Wawt;
    auto lineColor   = defaultOptions(WawtEnv::sPanel)
                          .lineColor(defaultOptions(WawtEnv::sScreen)
                                          .d_fillColor);
    auto layoutGrid  = gridLayoutGenerator(-1.0, 6, 1);
    auto layoutFn    =
        [layoutGrid]() mutable -> Layout {
            return layoutGrid().scale(1.0, 0.8);
        };
    auto screen =
        panel().addChild(
                    label({{-1.0, -1.0}, {1.0, -0.9}, 0.1},
                          S("Push Buttons & Grids"))
                    .options(defaultOptions(WawtEnv::sLabel)
                             .fillColor(DrawOptions::Color(235,235,255))))
               .addChild(
                    pushButtonGrid({{-1.0, 0.9}, {1.0, 1.0}}, -1.0, 2_Sz,
                                   {{d_prev, S("Prev")},
                                    {d_next, S("Next")}})
                                    .border(5).options(lineColor))
               .addChild(
                    panelLayout({{-1.0, 1.0, 0_wr}, {1.0, -1.0, 1_wr}},
                                layoutFn,

// Start Samples:
pushButton({}, FocusChgCb(), S("Click Me (1_Sz)"), 1_Sz),
pushButtonGrid({}, -1.0, 2_Sz,
               { {FocusChgCb(),S("Non-spaced Grid Choice 1 (2_Sz)")},
                 {FocusChgCb(),S("Non-spaced Grid Choice 2 (2_Sz)")} }, false),
pushButtonGrid({}, -1.0, 2_Sz,
               { {FocusChgCb(),S("Spaced Grid Choice 1 (2_Sz)")},
                 {FocusChgCb(),S("Spaced Grid Choice 2 (2_Sz)")} }),
pushButtonGrid({}, 2, -1.0, 3_Sz,
               { {FocusChgCb(),S("Non-spaced Grid Choice 1 (3_Sz)")},
                 {FocusChgCb(),S("Non-spaced Grid Choice 2 (3_Sz)")},
                 {FocusChgCb(),S("Choice 3 (3_Sz)")},
                 {FocusChgCb(),S("Choice 4 (3_Sz)")} }, false),
pushButtonGrid({}, 2, -1.0, 3_Sz,
               { {FocusChgCb(),S("Spaced Grid Choice 1 (3_Sz)")},
                 {FocusChgCb(),S("Spaced Grid Choice 2 (3_Sz)")},
                 {FocusChgCb(),S("Choice 3 (3_Sz)")},
                 {FocusChgCb(),S("Choice 4 (3_Sz)")} }, true),
pushButtonGrid({}, 3, -1.0, 3_Sz, { {FocusChgCb(),S("7")},
                             {FocusChgCb(),S("8")},
                             {FocusChgCb(),S("9")},
                             {FocusChgCb(),S("4")},
                             {FocusChgCb(),S("5")},
                             {FocusChgCb(),S("6")},
                             {FocusChgCb(),S("1")},
                             {FocusChgCb(),S("2")},
                             {FocusChgCb(),S("3")},
                             {FocusChgCb(),S("0")} }, true)));
// End Samples.
    return screen;                                                    // RETURN
}

#endif

// vim: ts=4:sw=4:et:ai

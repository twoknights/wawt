/** @file list.h
 *  @brief Fixed size list samples
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

#ifndef FACTORYSAMPLES_LIST_H
#define FACTORYSAMPLES_LIST_H

#include <drawoptions.h>
#include <wawt/layout.h>
#include <wawt/screen.h>
#include <wawt/wawtenv.h>
#include <wawt/widgetfactory.h>

#include <iostream>

                                //============
                                // class Lists
                                //============

class Lists : public Wawt::ScreenImpl<Lists,DrawOptions> {
  public:
    // PUBLIC TYPES

    // PUBLIC CONSTRUCTORS
    Lists(Wawt::FocusChgCb&& prev, Wawt::FocusChgCb&& next)
        : d_next(std::move(next)), d_prev(std::move(prev)) { }

    // PUBLIC MANIPULATORS
    // Called by 'WawtScreenImpl::setup()':
    Wawt::Widget createScreenPanel();

    // Called by 'WawtScreenImpl::activate()':
    void resetWidgets() {
        using namespace Wawt;
        // NOTE: single-select list retains last selection on prev+next.
        if (d_dropDown) {
            d_dropDown->children()[0].resetLabel(S("")); // but not drop-list
        }

        if (d_multiSelect) {
            for (auto& child : d_multiSelect->children()) {
                child.setSelected(false); // ... and not the mult-select list.
            }
        }
    }

private:
    // PRIVATE DATA MEMBERS
    Wawt::FocusChgCb d_next;
    Wawt::FocusChgCb d_prev;
    Wawt::Tracker    d_singleSelect;
    Wawt::Tracker    d_multiSelect;
    Wawt::Tracker    d_dropDown;
};

inline Wawt::Widget
Lists::createScreenPanel()
{
    using namespace Wawt;
    auto lineColor   = defaultOptions(WawtEnv::sPanel)
                          .lineColor(defaultOptions(WawtEnv::sScreen)
                                          .d_fillColor);
    auto layoutGrid  = gridLayoutGenerator(-1.0, 3, 3);
    auto layoutFn    =
        [layoutGrid]() mutable -> Layout {
            return layoutGrid().scale(0.8, 0.8);
        };
    auto screen =
        panel().addChild(
                    label({{-1.0, -1.0}, {1.0, -0.9}, 0.1},
                          S("Fixed Sized Lists"))
                    .options(defaultOptions(WawtEnv::sLabel)
                             .fillColor(DrawOptions::Color(235,235,255))))
               .addChild(
                    pushButtonGrid({{-1.0, 0.9}, {1.0, 1.0}}, -1.0, 2_Sz,
                                   {{d_prev, S("Prev")}, {d_next, S("Next")}})
                                    .border(5).options(lineColor))
               .addChild(
                    panelLayout({{-1.0, 1.0, 0_wr}, {1.0, -1.0, 1_wr}},
                                layoutFn,

// Start Samples:
fixedSizeList(d_singleSelect, {}, true, GridFocusCb(), 1_Sz,
              {S("Single Select"), S("Second"), S("Third"), S("Fourth")}),
fixedSizeList(d_multiSelect, {}, false, GridFocusCb(), 1_Sz,
              {S("Multi Select"), S("Second"), S("Third"), S("Fourth")}),
dropDownList(d_dropDown, {}, GridFocusCb(), 1_Sz,
              {S("First"), S("Second"), S("Third"), S("Fourth")})));
// End Samples.

    d_singleSelect->children()[2].disabled(true); // 'Third' disabled
    d_multiSelect ->children()[3].disabled(true); // 'Fourth' disabled
    return screen;                                                    // RETURN
}

#endif

// vim: ts=4:sw=4:et:ai

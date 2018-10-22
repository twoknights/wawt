/** @file bullet.h
 *  @brief Check boxes and radio button groups
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

#ifndef FACTORYSAMPLES_BULLET_H
#define FACTORYSAMPLES_BULLET_H

#include <drawoptions.h>
#include <wawt/layout.h>
#include <wawt/screen.h>
#include <wawt/wawtenv.h>
#include <wawt/widgetfactory.h>

#include <iostream>

                                //==============
                                // class Bullets
                                //==============

class Bullets : public Wawt::ScreenImpl<Bullets,DrawOptions> {
  public:
    // PUBLIC TYPES

    // PUBLIC CONSTRUCTORS
    Bullets(Wawt::FocusChgCb&& prev, Wawt::FocusChgCb&& next)
        : d_next(std::move(next)), d_prev(std::move(prev)) { }

    // PUBLIC MANIPULATORS
    // Called by 'WawtScreenImpl::setup()':
    Wawt::Widget createScreenPanel();

    // Called by 'WawtScreenImpl::activate()':
    void resetWidgets() { }

private:
    // PRIVATE DATA MEMBERS
    Wawt::FocusChgCb d_next;
    Wawt::FocusChgCb d_prev;
};

inline Wawt::Widget
Bullets::createScreenPanel()
{
    using namespace Wawt;
    auto lineColor   = defaultOptions(WawtEnv::sPanel)
                          .lineColor(defaultOptions(WawtEnv::sScreen)
                                          .d_fillColor);
    // 4 examples in panels layed-out in quadrants and then "shrunk" by 20%
    // so they don't share common borders:
    auto layoutGrid  = gridLayoutGenerator(0.0, 2, 4);
    auto layoutFn    =
        [layoutGrid]() mutable -> Layout {
            return layoutGrid().scale(0.8, 0.8);
        };

    auto screen =
        panel().addChild(
                    label({{-1.0, -1.0}, {1.0, -0.9}, 0.1},
                          S("Check Boxes & Radio Button Groups"))
                    .options(defaultOptions(WawtEnv::sLabel)
                             .fillColor(DrawOptions::Color(235,235,255))))
               .addChild(
                    pushButtonGrid({{-1.0, 0.9}, {1.0, 1.0}, 0.1}, 2_F, 2,
                                   {{d_prev, S("Prev")},
                                    {d_next, S("Next")}}, true)
                                    .border(10).options(lineColor))
               .addChild(
                    layoutPanel({{-1.0, 1.0, 0_wr}, {1.0, -1.0, 1_wr}},
                                layoutFn,

// Start Samples:
checkBox({}, S("Left alignment."), 1_F),
checkBox({}, S("Right alignment."), 1_F, TextAlign::eRIGHT),
radioButtonPanel({}, GridFocusCb(), 1_F, { S("A"), S("B"), S("C") }),
radioButtonPanel({}, GridFocusCb(), 1_F, { S("A"), S("B"), S("C"), S("D") },
                 TextAlign::eRIGHT, 2)));
// End Samples.

    return screen;                                                    // RETURN
}

#endif

// vim: ts=4:sw=4:et:ai

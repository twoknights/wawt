/** @file panel.h
 *  @brief Panel Samples
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

#ifndef FACTORYSAMPLES_PANELS_H
#define FACTORYSAMPLES_PANELS_H

#include <drawoptions.h>
#include <wawt/layout.h>
#include <wawt/screen.h>
#include <wawt/wawtenv.h>
#include <wawt/widgetfactory.h>

#include <iostream>

                                //=============
                                // class Panels
                                //=============

class Panels : public Wawt::ScreenImpl<Panels,DrawOptions> {
  public:
    // PUBLIC TYPES

    // PUBLIC CONSTRUCTORS
    Panels() { }

    // PUBLIC MANIPULATORS
    // Called by 'WawtScreenImpl::setup()':
    Wawt::Widget createScreenPanel();

    // Called by 'WawtScreenImpl::activate()':
    void resetWidgets() { }

private:
    // PRIVATE DATA MEMBERS
};

inline Wawt::Widget
Panels::createScreenPanel()
{
    using namespace Wawt;
    auto screen =
        panel().addChild(
                label({{-1.0,-1.0},{1.0,-.9},0.1}, S("Panels"))
                 .options(defaultOptions(WawtEnv::sLabel)
                            .fillColor(DrawOptions::Color(235,235,255))))
               .addChild(panelGrid({{-1.0,1.0,0_wr},{1.0,1.0},0.0},
                                   2, 3,
                                   panel(Layout::centered(0.5,0.5))
                                    .addChild(label(Layout::centered(0.8,0.2),
                                                    S("Label"), 1_F))));
    auto& panels = screen.children()[1].children();
    panels[0].children()[0].resetLabel(S("Default Panel"));
    panels[1].children()[0].resetLabel(S("1% Border"));
    panels[1].layoutData().d_layout.d_percentBorder = 1.0;
    panels[2].children()[0].resetLabel(S("1% Border & Fill Color"));
    panels[2].layoutData().d_layout.d_percentBorder = 1.0;
    panels[2].setOptions(defaultOptions(WawtEnv::sPanel)
                        .fillColor(DrawOptions::Color(192u, 192u, 255u)));
    return screen;                                                    // RETURN
}

#endif

// vim: ts=4:sw=4:et:ai

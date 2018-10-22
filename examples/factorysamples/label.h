/** @file label.h
 *  @brief Label Samples
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

#ifndef FACTORYSAMPLES_LABELS_H
#define FACTORYSAMPLES_LABELS_H

#include <drawoptions.h>
#include <wawt/layout.h>
#include <wawt/screen.h>
#include <wawt/wawtenv.h>
#include <wawt/widgetfactory.h>

#include <iostream>

                                //=============
                                // class Labels
                                //=============

class Labels : public Wawt::ScreenImpl<Labels,DrawOptions> {
  public:
    // PUBLIC TYPES

    // PUBLIC CONSTRUCTORS
    Labels(Wawt::FocusChgCb&& next) : d_next(std::move(next)) { }

    // PUBLIC MANIPULATORS
    // Called by 'WawtScreenImpl::setup()':
    Wawt::Widget createScreenPanel();

    // Called by 'WawtScreenImpl::activate()':
    void resetWidgets() { }

private:
    // PRIVATE DATA MEMBERS
    Wawt::FocusChgCb d_next;
};

inline Wawt::Widget
Labels::createScreenPanel()
{
    using namespace Wawt;
    auto lineColor = defaultOptions(WawtEnv::sPanel)
                        .lineColor(defaultOptions(WawtEnv::sScreen)
                                        .d_fillColor);
    auto screen =
        panel().addChild(
                    label({{-1.0, -1.0}, {1.0, -0.9}, 0.1}, S("Labels"))
                    .options(defaultOptions(WawtEnv::sLabel)
                             .fillColor(DrawOptions::Color(235,235,255))))
               .addChild(
                    pushButtonGrid({{-1.0, 0.9}, {1.0, 1.0}, 0.1}, 2_F, 2,
                                   {{d_next, S("Next")}}, true)
                                    .border(10).options(lineColor))
               .addChild(
                    layoutPanel({{-1.0, 1.0, 0_wr}, {1.0, -1.0, 1_wr}}, 0, 1,

// Start Samples:
label({}, S("The default label has no border,")),
label({}, S("no fill color,")),
label({}, S("centered; with font size selected so the label fits.")),
label({}, S("Labels can be 'left' aligned,"), TextAlign::eLEFT, 1_F),
label({}, S("or 'right' aligned,"), TextAlign::eRIGHT, 1_F),
label({}, S("and assigned to a font size group where all share the same size."),
    1_F),
label({}, S("С поддержкой UTF-8 или широким символом."))));
// End Samples.

    return screen;                                                    // RETURN
}

#endif

// vim: ts=4:sw=4:et:ai

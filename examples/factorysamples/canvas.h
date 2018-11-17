/** @file canvas.h
 *  @brief Canvas Sample
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

#ifndef FACTORYSAMPLES_CANVAS_H
#define FACTORYSAMPLES_CANVAS_H

#include <drawoptions.h>
#include <wawt/layout.h>
#include <wawt/screen.h>
#include <wawt/wawtenv.h>
#include <wawt/widgetfactory.h>

#include <iostream>

                                //=============
                                // class Canvas
                                //=============

class Canvas : public Wawt::ScreenImpl<Canvas,DrawOptions> {
  public:
    // PUBLIC TYPES

    // PUBLIC CONSTRUCTORS
    Canvas(Wawt::OnClickCb&& prev) : d_prev(std::move(prev)) { }

    // PUBLIC MANIPULATORS

    // Called by 'WawtScreenImpl::setup()':
    Wawt::Widget createScreenPanel();

    // Called by 'WawtScreenImpl::activate()':
    void resetWidgets() { }

private:
    // PRIVATE DATA MEMBERS
    Wawt::OnClickCb d_prev;
    double          d_x = -1.f;
    double          d_y = -1.f;
};

inline Wawt::Widget
Canvas::createScreenPanel()
{
    using namespace Wawt;
    using namespace Wawt::literals;
    extern void drawHexBoard(Widget       *widget,
                             DrawProtocol *adapter,
                             double        x,
                             double        y);

    auto lineColor  = defaultOptions(WawtEnv::sPanel)
                        .lineColor(defaultOptions(WawtEnv::sScreen)
                                        .d_fillColor);
    auto heading = S("Raw Drawing (SFML) & Click Handling with Canvas");
    auto screen =
        panel().addChild(
                    label({{-1.0, -1.0}, {1.0, -0.9}, 0.1}, heading)
                    .downEventMethod(&dumpScreen)
                    .options(defaultOptions(WawtEnv::sLabel)
                             .fillColor(DrawOptions::Color(235,235,255))))
               .addChild(
                    pushButtonGrid({{-1.0, 0.9}, {1.0, 1.0}}, -1.0, 1_Sz,
                                   {{S("Prev"),    d_prev}})
                    .border(5).options(lineColor))
               .addChild(
                    canvas({{-1.0, 1.25, 0_wr}, {1.0, -1.25, 1_wr}},
                           [this](Widget *widget, DrawProtocol *adapter) {
                                drawHexBoard(widget, adapter, d_x, d_y);
                           },
                           [me = this](double x, double y, Widget*, Widget*) {
                                me->d_x = x; me->d_y = y;
                                return [me](double mx, double my, bool up) {
                                    if (up) {
                                        me->d_x = -1.0; me->d_y = -1.0;
                                    }
                                    else {
                                        me->d_x = mx; me->d_y = my;
                                    }
                                };
                           }));

    return screen;                                                    // RETURN
}

#endif

// vim: ts=4:sw=4:et:ai

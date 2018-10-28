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
    Panels(Wawt::OnClickCb&& prev, Wawt::OnClickCb&& next)
        : d_next(std::move(next)), d_prev(std::move(prev)) { }

    // PUBLIC MANIPULATORS
    // Called by 'WawtScreenImpl::setup()':
    Wawt::Widget createScreenPanel();

    // Called by 'WawtScreenImpl::activate()':
    void resetWidgets() { }

private:
    // PRIVATE DATA MEMBERS
    Wawt::OnClickCb d_next;
    Wawt::OnClickCb d_prev;
};

inline Wawt::Widget
Panels::createScreenPanel()
{
    using namespace Wawt;
    auto panelFill   = defaultOptions(WawtEnv::sPanel)
                          .fillColor(DrawOptions::Color(192u, 192u, 255u));
    auto lineColor   = defaultOptions(WawtEnv::sPanel)
                          .lineColor(defaultOptions(WawtEnv::sScreen)
                                          .d_fillColor);
    auto layoutGrid  = gridLayoutGenerator(0.0, 4, 2);
    auto layoutFn    =
        [layoutGrid, n = 0]() mutable -> Layout {
            return layoutGrid().scale(0.8, 0.8).border(n++ % 2 ? 5.0 : -1.0);
        };
    auto popDialog =
        [me = this](Widget *) -> void {
            auto buttons = pushButtonGrid({}, 2.0, kNOGROUP,
                            {
                                {   [me](Widget *) -> void {
                                        me->dropModalDialogBox();
                                    },
                                    S("Close")
                                }
                            });
            auto dialog = dialogBox(
                    Layout().scale(0.33, 0.33),
                    std::move(buttons),
                    { // Note: the 2_Sz here is not the same as the one below
                        {S("Pop-up dialogs...")},
                        {S("... are panels too! Only the 'Close'"), 2_Sz},
                        {S("button is active on the screen."), 2_Sz}
                    });
            me->addModalDialogBox(std::move(dialog));
        };
    auto screen =
        panel().addChild(
                    label({{-1.0, -1.0}, {1.0, -0.9}, 0.1}, S("Panels"))
                    .options(defaultOptions(WawtEnv::sLabel)
                             .fillColor(DrawOptions::Color(235,235,255))))
               .addChild(
                    pushButtonGrid({{-1.0, 0.9}, {1.0, 1.0}}, -1.0, 2_Sz,
                                   {{d_prev,    S("Prev")},
                                    {popDialog, S("Dialog")},
                                    {d_next,    S("Next")}})
                                    .border(5).options(lineColor))
               .addChild(
                    panelLayout({{-1.0, 1.0, 0_wr}, {1.0, -1.0, 1_wr}},
                                layoutFn,

// Start Samples:
panel({}).text(S("Default Panel"), 1_Sz), // Default panels have black text
panel({}).text(S("+ 5% Border"), 1_Sz),
panel({}, panelFill).text(S("+ Fill Option"), 1_Sz),
panel({}, panelFill).text(S("+ 5% & Fill Option"), 1_Sz)));
// End Samples.

    return screen;                                                    // RETURN
}

#endif

// vim: ts=4:sw=4:et:ai

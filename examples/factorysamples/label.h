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

#ifndef FACTORYSAMPLES_LABEL_H
#define FACTORYSAMPLES_LABEL_H

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
    enum LabelId { OtherLanguage };

    struct Translate : public Wawt::WawtEnv::Translator {
        Wawt::StringView_t operator()(const Wawt::StringView_t& string) {
            if (0 == string.compare("With support for")) {
                return d_english ? string : S("С поддержкой");
            }
            if (0 == string.compare("or wide character strings.")) {
                return d_english ? string : S("или широких символьных строк.");
            }
            return string;
        }

        Wawt::StringView_t operator()(int id) {
            if (static_cast<LabelId>(id) == OtherLanguage) {
                return d_english ? S("Pусский") : S("English");
            }
            return Wawt::StringView_t();
        }

        bool            d_english = false;
    };

    // PUBLIC CONSTRUCTORS
    Labels(Wawt::OnClickCb&& next, bool& english)
        : d_next(std::move(next)), d_english(english) { }

    // PUBLIC MANIPULATORS
    // Called by 'WawtScreenImpl::setup()':
    Wawt::Widget createScreenPanel();

    // Called by 'WawtScreenImpl::activate()':
    void resetWidgets() { }

private:
    // PRIVATE DATA MEMBERS
    Wawt::OnClickCb d_next;
    bool&           d_english;
};

inline Wawt::Widget
Labels::createScreenPanel()
{
    using namespace Wawt;
    using namespace Wawt::literals;
    auto yellowText = defaultOptions(WawtEnv::sLabel)
                        .textColor({255u,255u,0u});
    auto lineColor  = defaultOptions(WawtEnv::sPanel)
                        .lineColor(defaultOptions(WawtEnv::sScreen)
                                        .d_fillColor);
    auto toggle = [this] (Widget*) {
        d_english = !d_english;
        synchronizeTextView();
        resize();
    };
    auto screen =
        panel().addChild(
                    label({{-1.0, -1.0}, {1.0, -0.9}, 0.1}, S("Labels"))
                    .downEventMethod(&dumpScreen)
                    .options(defaultOptions(WawtEnv::sLabel)
                             .fillColor(DrawOptions::Color(235,235,255))))
               .addChild(
                    pushButtonGrid({{-1.0, 0.9}, {1.0, 1.0}}, -1.0, 1_Sz,
                                   {{LabelId::OtherLanguage, toggle},
                                    {S("Next"),    d_next}})
                    .border(5).options(lineColor))
               .addChild(
                    panelLayout({{-1.0, 1.0, 0_wr}, {1.0, -1.0, 1_wr}}, 0, 1,

// Start Samples:
label({}, S("The default label has no border,")),
label({}, S("no fill color,")),
label({}, S("centered; with font size selected so the label fits.")),
label({}, S("Labels can be 'left' aligned,"), 2_Sz, TextAlign::eLEFT),
label({}, S("or 'right' aligned,"), 2_Sz, TextAlign::eRIGHT),
label({},
  S("and assigned to a character size group where all share the same size."),
      2_Sz),
concatenateLabels({}, 3_Sz, TextAlign::eCENTER, {
        { S("With support for"), yellowText},
        { S(" UTF-8 "), yellowText.clone().bold(true).font(1)
                                          .textColor({255u,0u,0u})},
        { S("or wide character strings."), yellowText}
    })));
// End Samples.

    return screen;                                                    // RETURN
}

#endif

// vim: ts=4:sw=4:et:ai

/** @file widgetfactory.cpp
 *  @brief Factory methods to create different widget "classes".
 *
 * Copyright 2018 Bruce Szablak

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

#include "wawt/widgetfactory.h"


#ifdef WAWT_WIDECHAR
#define S(str) String_t(L"" str)  // wide char strings (std::wstring)
#define C(c) (L ## c)
#else
#undef  S
#undef  C
#define S(str) String_t(u8"" str)      // UTF8 strings  (std::string)
#define C(c) (U ## c)
#endif

namespace Wawt {

namespace {

Widget::DownEventMethod makePushButtonDownMethod(ClickCb&& onClick)
{
    return  [cb=std::move(onClick)] (auto, auto, auto *widget) -> EventUpCb {
                widget->drawData().d_selected = true;
                return  [cb, widget](float x, float y, bool up) -> FocusCb {
                            widget->drawData().d_selected = false;
                            if (cb) {
                                if (up && widget->inside(x, y)) {
                                    cb(widget);
                                }
                            }
                            return FocusCb();
                        };
            };                                                        // RETURN
}

Widget::DownEventMethod makeToggleButtonDownMethod(ClickCb&& onClick)
{
    return  [cb=std::move(onClick)] (auto, auto, auto *widget) -> EventUpCb {
                return  [cb, widget](float x, float y, bool up) -> FocusCb {
                            if (up && widget->inside(x, y)) {
                                widget->drawData().d_selected =
                                                !widget->drawData().d_selected;
                                if (cb) {
                                    cb(widget);
                                }
                            }
                            return FocusCb();
                        };
            };                                                        // RETURN
}

} // unnamed namespace

Widget checkBox(Widget                **indirect,
                Layout&&                layout,
                StringView_t            string,
                Text::CharSizeGroup     group,
                ClickCb                 clicked,
                Text::Align             alignment,
                bool                    leftBox)
{
    return  Widget(WawtEnv::sCheck, indirect, std::move(layout))
            .addMethod(makeToggleButtonDownMethod(std::move(clicked)))
            .labelSelect(layout.d_thickness <= 0.0)
            .text({string, group, alignment},
                   DrawData::BulletMark::eSQUARE,
                   leftBox);                                          // RETURN
}

Widget checkBox(Layout&&                layout,
                StringView_t            string,
                Text::CharSizeGroup     group,
                ClickCb                 clicked,
                Text::Align             alignment,
                bool                    leftBox)
{
    return  Widget(WawtEnv::sCheck, std::move(layout))
            .addMethod(makeToggleButtonDownMethod(std::move(clicked)))
            .labelSelect(layout.d_thickness <= 0.0)
            .text({string, group, alignment},
                   DrawData::BulletMark::eSQUARE,
                   leftBox);                                          // RETURN
}

Widget checkBox(Widget                **indirect,
                Layout&&                layout,
                StringView_t            string,
                Text::CharSizeGroup     group,
                Text::Align             alignment)
{
    return  Widget(WawtEnv::sCheck, indirect, std::move(layout))
            .addMethod(makeToggleButtonDownMethod(ClickCb()))
            .labelSelect(layout.d_thickness <= 0.0)
            .text({string, group, alignment},
                   DrawData::BulletMark::eSQUARE,
                   alignment != Text::Text::Align::eRIGHT);           // RETURN
}

Widget checkBox(Layout&&                layout,
                StringView_t            string,
                Text::CharSizeGroup     group,
                Text::Align             alignment)
{
    return  Widget(WawtEnv::sCheck, std::move(layout))
            .addMethod(makeToggleButtonDownMethod(ClickCb()))
            .labelSelect(layout.d_thickness <= 0.0)
            .text({string, group, alignment},
                   DrawData::BulletMark::eSQUARE,
                   alignment != Text::Text::Align::eRIGHT);           // RETURN
}

Widget label(Widget                   **indirect,
             Layout&&                   layout,
             StringView_t               string,
             Text::Align                alignment,
             Text::CharSizeGroup        group)
{
    return  Widget(WawtEnv::sLabel, indirect, std::move(layout))
            .text({string, group, alignment});                        // RETURN
}

Widget label(Layout&&                   layout,
             StringView_t               string,
             Text::Align                alignment,
             Text::CharSizeGroup        group)
{
    return  Widget(WawtEnv::sLabel, std::move(layout))
            .text({string, group, alignment});                        // RETURN
}

Widget label(Widget                   **indirect,
             Layout&&                   layout,
             StringView_t               string,
             Text::CharSizeGroup        group)
{
    return  Widget(WawtEnv::sLabel, indirect, std::move(layout))
            .text({string, group});                                   // RETURN
}

Widget label(Layout&& layout, StringView_t string, Text::CharSizeGroup group)
{
    return  Widget(WawtEnv::sLabel, std::move(layout))
            .text({string, group});                                   // RETURN
}

Widget panel(Widget **indirect, Layout&& layout)
{
    return Widget(WawtEnv::sPanel, indirect, std::move(layout));      // RETURN
}

Widget panel(Layout&& layout)
{
    return Widget(WawtEnv::sPanel, std::move(layout));                // RETURN
}

Widget panelGrid(Widget       **indirect,
                 Layout&&       layout,
                 int            rows,
                 int            columns,
                 Widget&&       clonable)
{
    auto thickness = clonable.layoutData().d_layout.d_thickness;
    auto panel     = Widget(WawtEnv::sPanel,
                            indirect,
                            std::move(layout).border(0.0));

    auto topLeft = panel.addChild(std::move(clonable));
    topLeft->layoutData().d_layout
                    = {{-1.0, -1.0}, // using panel's coordinates.
                       {-1.0 + 2.0/double(columns), -1.0 + 2.0/double(rows)},
                       thickness};
    // All remaining widgets will use the coordinate system of a neighbor.
    auto col0Id = WidgetId();

    for (auto r = 0; r < rows; ++r) {
        if (r > 0) {
            panel.addChild(topLeft->clone())->layoutData().d_layout
                = {{-1.0, 1.0, col0Id}, { 1.0, 2.0, col0Id}, thickness};
        }
        col0Id        = WidgetId(r*columns, true);
        WidgetId prev = col0Id;

        for (auto c = 1; c < columns; ++c, ++prev) {
            panel.addChild(topLeft->clone())->layoutData().d_layout
                = {{ 1.0, -1.0, prev}, { 3.0, 1.0, prev }, thickness};
        }
    }
    return panel;                                                     // RETURN
}

Widget panelGrid(Layout&&       layout,
                 int            rows,
                 int            columns,
                 Widget&&       cloneable)
{
    return panelGrid(nullptr,
                     std::move(layout),
                     rows, columns,
                     std::move(cloneable));
}

Widget pushButton(Widget              **indirect,
                  Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  Text::Align           alignment,
                  Text::CharSizeGroup   group)
{
    return  Widget(WawtEnv::sPush, indirect, std::move(layout))
            .addMethod(makePushButtonDownMethod(std::move(clicked)))
            .text({string, group, alignment});                        // RETURN
}

Widget pushButton(Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  Text::Align           alignment,
                  Text::CharSizeGroup   group)
{
    return  Widget(WawtEnv::sPush, std::move(layout))
            .addMethod(makePushButtonDownMethod(std::move(clicked)))
            .text({string, group, alignment});                        // RETURN
}

Widget pushButton(Widget              **indirect,
                  Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  Text::CharSizeGroup   group)
{
    return  Widget(WawtEnv::sPush, indirect, std::move(layout))
            .addMethod(makePushButtonDownMethod(std::move(clicked)))
            .text({string, group});                                   // RETURN
}

Widget pushButton(Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  Text::CharSizeGroup   group)
{
    return  Widget(WawtEnv::sPush, std::move(layout))
            .addMethod(makePushButtonDownMethod(std::move(clicked)))
            .text({string, group});                                   // RETURN
}

}  // namespace Wawt
// vim: ts=4:sw=4:et:ai

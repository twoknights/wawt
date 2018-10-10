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
#define S(str) Wawt::String_t(L"" str)  // wide char strings (std::wstring)
#define C(c) (L ## c)
#else
#undef  S
#undef  C
#define S(str) Wawt::String_t(u8"" str)      // UTF8 strings  (std::string)
#define C(c) (U ## c)
#endif

namespace Wawt {

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

Widget checkBox(Widget                **indirect,
                Layout&&                layout,
                StringView_t            string,
                Text::CharSizeGroup     group,
                ClickCb                 clicked,
                Text::Align             alignment,
                bool                    leftBox)
{
    return  Widget(Wawt::sCheck, indirect, std::move(layout))
            .downEventMethod(makeToggleButtonDownMethod(std::move(clicked)),
                             layout.d_thickness <= 0.0)
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
    return  Widget(Wawt::sCheck, std::move(layout))
            .downEventMethod(makeToggleButtonDownMethod(std::move(clicked)),
                             layout.d_thickness <= 0.0)
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
    return  Widget(Wawt::sCheck, indirect, std::move(layout))
            .downEventMethod(makeToggleButtonDownMethod(ClickCb()),
                             layout.d_thickness <= 0.0)
            .text({string, group, alignment},
                   DrawData::BulletMark::eSQUARE,
                   alignment != Text::Text::Align::eRIGHT);           // RETURN
}

Widget checkBox(Layout&&                layout,
                StringView_t            string,
                Text::CharSizeGroup     group,
                Text::Align             alignment)
{
    return  Widget(Wawt::sCheck, std::move(layout))
            .downEventMethod(makeToggleButtonDownMethod(ClickCb()),
                             layout.d_thickness <= 0.0)
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
    return  Widget(Wawt::sLabel, indirect, std::move(layout))
            .text({string, group, alignment});                        // RETURN
}

Widget label(Layout&&                   layout,
             StringView_t               string,
             Text::Align                alignment,
             Text::CharSizeGroup        group)
{
    return  Widget(Wawt::sLabel, std::move(layout))
            .text({string, group, alignment});                        // RETURN
}

Widget label(Widget                   **indirect,
             Layout&&                   layout,
             StringView_t               string,
             Text::CharSizeGroup        group)
{
    return  Widget(Wawt::sLabel, indirect, std::move(layout))
            .text({string, group});                                   // RETURN
}

Widget label(Layout&& layout, StringView_t string, Text::CharSizeGroup group)
{
    return  Widget(Wawt::sLabel, std::move(layout))
            .text({string, group});                                   // RETURN
}

Widget panel(Widget **indirect, Layout&& layout)
{
    return Widget(Wawt::sPanel, indirect, std::move(layout));         // RETURN
}

Widget panel(Layout&& layout)
{
    return Widget(Wawt::sPanel, std::move(layout));                   // RETURN
}

Widget pushButton(Widget              **indirect,
                  Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  Text::Align           alignment,
                  Text::CharSizeGroup   group)
{
    return  Widget(Wawt::sPush, indirect, std::move(layout))
            .downEventMethod(makePushButtonDownMethod(std::move(clicked)))
            .text({string, group, alignment});                        // RETURN
}

Widget pushButton(Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  Text::Align           alignment,
                  Text::CharSizeGroup   group)
{
    return  Widget(Wawt::sPush, std::move(layout))
            .downEventMethod(makePushButtonDownMethod(std::move(clicked)))
            .text({string, group, alignment});                        // RETURN
}

Widget pushButton(Widget              **indirect,
                  Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  Text::CharSizeGroup   group)
{
    return  Widget(Wawt::sPush, indirect, std::move(layout))
            .downEventMethod(makePushButtonDownMethod(std::move(clicked)))
            .text({string, group});                                   // RETURN
}

Widget pushButton(Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  Text::CharSizeGroup   group)
{
    return  Widget(Wawt::sPush, std::move(layout))
            .downEventMethod(makePushButtonDownMethod(std::move(clicked)))
            .text({string, group});                                   // RETURN
}

}  // namespace Wawt
// vim: ts=4:sw=4:et:ai

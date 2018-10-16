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
#include "wawt/wawtenv.h"

#include <memory>
#include <utility>

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

Widget::DownEventMethod makePushButtonDownMethod(FocusChgCb&& onClick)
{
    return  [cb=std::move(onClick)] (auto, auto, auto *widget) -> EventUpCb {
                widget->drawData().d_selected = true;
                return  [cb, widget](float x, float y, bool up) -> FocusCb {
                            widget->drawData().d_selected = false;
                            if (cb) {
                                if (up && widget->inside(x, y)) {
                                    return cb(widget);
                                }
                            }
                            return FocusCb();
                        };
            };                                                        // RETURN
}

Widget::DownEventMethod makeToggleButtonDownMethod(ClickCb&& onClick)
{
    return  [clk=std::move(onClick)] (auto, auto, auto *widget) -> EventUpCb {
                return  [clk, widget](float x, float y, bool up) -> FocusCb {
                            if (up && widget->inside(x, y)) {
                                widget->drawData().d_selected =
                                                !widget->drawData().d_selected;
                                if (clk) {
                                    clk(widget);
                                }
                            }
                            return FocusCb();
                        };
            };                                                        // RETURN
}

using DimensionPtr = std::shared_ptr<Dimensions>;

Widget::LayoutMethod genSpacedLayout(const DimensionPtr&   bounds,
                                     int                   rows,
                                     int                   columns,
                                     TextAlign             alignment)
{
    return
        [=] (Widget                 *widget,
             const Widget&           parent,
             const Widget&           root,
             bool                    firstPass,
             DrawProtocol           *adapter) -> void {
            Widget::defaultLayout(widget, parent, root, firstPass, adapter);

            auto& data = widget->drawData();

            if (firstPass) {
                if (data.d_relativeId == 0 // ignore old values
                 || data.d_labelBounds.d_width > bounds->d_width) {
                    bounds->d_width = data.d_labelBounds.d_width;
                }

                if (data.d_relativeId == 0
                 || data.d_labelBounds.d_height > bounds->d_height) {
                    bounds->d_height = data.d_labelBounds.d_height;
                }
                return;
            }
            auto offset      = 2*data.d_rectangle.d_borderThickness + 2;
            auto panelWidth  = parent.drawData().d_rectangle.d_width;
            auto panelHeight = parent.drawData().d_rectangle.d_height;
            auto width       = bounds->d_width  + offset;
            auto height      = bounds->d_height + offset;
            // Calculate seperator spacing between buttons:
            auto spacex = columns == 1
                ? 0
                : (panelWidth - columns*width)/(columns - 1);
            auto spacey = rows == 1
                ? 0
                : (panelHeight - rows*height)/(rows - 1);

            // But don't seperate them by too much:
            if (spacex > width/2) {
                spacex = width/2;
            }

            if (spacey > height/2) {
                spacey = height/2;
            }
            // ... which might mean the panel needs margins:
            auto marginx = (panelWidth - columns*width - (columns-1)*spacex)/2;
            auto marginy = (panelHeight- rows * height -    (rows-1)*spacey)/2;
            auto c       = data.d_relativeId % columns;
            auto r       = data.d_relativeId / columns;
            
            // Translate the button at row 'r' and column 'c'
            auto& rectangle     = data.d_rectangle;
            rectangle.d_width   = width;
            rectangle.d_height  = height;
            rectangle.d_ux      = parent.drawData().d_rectangle.d_ux
                                        + marginx + c * (spacex+width);
            rectangle.d_uy      = parent.drawData().d_rectangle.d_uy
                                        + marginy + r * (spacey+height);
            auto& label         = data.d_labelBounds;
            label.d_uy          = rectangle.d_uy + (height-label.d_height)/2.0;
            label.d_ux          = rectangle.d_ux + offset/2.0;

            if (alignment != TextAlign::eLEFT) {
                auto space = width - label.d_width - offset;

                if (alignment == TextAlign::eCENTER) {
                    space /= 2.0;
                }
                label.d_ux += space;
            }
        };                                                            // RETURN
}

} // unnamed namespace

Widget bulletButtonGrid(Widget                **indirect,
                        Layout&&                layout,
                        bool                    radioButtons,
                        SelectCb&&              selectCb,
                        CharSizeGroup           group,
                        LabelList               labels,
                        TextAlign               alignment,
                        int                     columns)
{
    auto gridPanel   = panel(indirect, std::move(layout));
#if 0
    auto rows        = std::size_t{};
    auto childLayout = gridLayoutSequencer(columns, labels.size(), &rows);

    for (auto& label : labels) {
        auto onClick =...;
        auto button
            = Widget(WawtEnv::sBullet, indirect, childLayout())
                .addMethod(onClick)
                .labelSelect(true)
                .text(label, group, alignment)
                .textMark(radioButtons ? DrawData::BulletMark::eROUND
                                       : DrawData::BulletMark::eSQUARE,
                          alignment != TextAlign::eRIGHT);

        gridPanel.addChild(button);
    }
#endif
    return gridPanel;                                                 // RETURN
}

Widget bulletButtonGrid(Layout&&                layout,
                        bool                    radioButtons,
                        SelectCb&&              selectCb,
                        CharSizeGroup           group,
                        LabelList               labels,
                        TextAlign               alignment,
                        int                     columns)
{
    return bulletButtonGrid(nullptr, std::move(layout), radioButtons,
                            std::move(selectCb), group, labels, alignment,
                            columns);
}

Widget bulletButtonGrid(Widget                **indirect,
                        Layout&&                layout,
                        bool                    radioButtons,
                        SelectFocusCb&&         selectCb,
                        CharSizeGroup           group,
                        LabelList               labels,
                        TextAlign               alignment,
                        int                     columns)
{
    return panel({});                                                 // RETURN
}

Widget bulletButtonGrid(Layout&&                layout,
                        bool                    radioButtons,
                        SelectFocusCb&&         selectCb,
                        CharSizeGroup           group,
                        LabelList               labels,
                        TextAlign               alignment,
                        int                     columns)
{
    return bulletButtonGrid(nullptr, std::move(layout), radioButtons,
                            std::move(selectCb), group, labels, alignment,
                            columns);
}

Widget checkBox(Widget                **indirect,
                Layout&&                layout,
                StringView_t            string,
                CharSizeGroup           group,
                TextAlign               alignment)
{
    return  Widget(WawtEnv::sBullet, indirect, std::move(layout))
            .addMethod(makeToggleButtonDownMethod(ClickCb()))
            .labelSelect(layout.d_thickness <= 0.0)
            .text(string, group, alignment)
            .textMark(DrawData::BulletMark::eSQUARE,
                      alignment != TextAlign::eRIGHT);        // RETURN
}

Widget checkBox(Layout&&                layout,
                StringView_t            string,
                CharSizeGroup           group,
                TextAlign               alignment)
{
    return  Widget(WawtEnv::sBullet, std::move(layout))
            .addMethod(makeToggleButtonDownMethod(ClickCb()))
            .labelSelect(layout.d_thickness <= 0.0)
            .text(string, group, alignment)
            .textMark(DrawData::BulletMark::eSQUARE,
                      alignment != TextAlign::eRIGHT);        // RETURN
}

Widget label(Widget                   **indirect,
             Layout&&                   layout,
             StringView_t               string,
             TextAlign                  alignment,
             CharSizeGroup              group)
{
    return  Widget(WawtEnv::sLabel, indirect, std::move(layout))
            .text(string, group, alignment);                          // RETURN
}

Widget label(Layout&&                   layout,
             StringView_t               string,
             TextAlign                  alignment,
             CharSizeGroup              group)
{
    return  Widget(WawtEnv::sLabel, std::move(layout))
            .text(string, group, alignment);                          // RETURN
}

Widget label(Widget                   **indirect,
             Layout&&                   layout,
             StringView_t               string,
             CharSizeGroup              group)
{
    return  Widget(WawtEnv::sLabel, indirect, std::move(layout))
            .text(string, group);                                     // RETURN
}

Widget label(Layout&& layout, StringView_t string, CharSizeGroup         group)
{
    return  Widget(WawtEnv::sLabel, std::move(layout))
            .text(string, group);                                     // RETURN
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
                 const Widget&  clonable)
{
    auto thickness = clonable.layoutData().d_layout.d_thickness;
    auto grid      = panel(indirect, std::move(layout).border(0.0));
    auto layoutFn  = gridLayoutSequencer(columns, rows*columns);

    for (auto i = 0; i < rows*columns; ++i) {
        auto child = clonable.clone();
        child.layoutData().d_layout = layoutFn();
        grid.addChild(std::move(child));
    }
    return grid;                                                      // RETURN
}

Widget panelGrid(Layout&&       layout,
                 int            rows,
                 int            columns,
                 const Widget&  cloneable)
{
    return panelGrid(nullptr,
                     std::move(layout),
                     rows, columns,
                     std::move(cloneable));                           // RETURN
}

Widget pushButton(Widget              **indirect,
                  Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  TextAlign             alignment,
                  CharSizeGroup         group)
{
    return  Widget(WawtEnv::sPush, indirect, std::move(layout))
            .addMethod(makePushButtonDownMethod(std::move(clicked)))
            .text(string, group, alignment);                          // RETURN
}

Widget pushButton(Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  TextAlign             alignment,
                  CharSizeGroup         group)
{
    return pushButton(nullptr, std::move(layout),
                      clicked, string, alignment, group);             // RETURN
}

Widget pushButton(Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  CharSizeGroup         group)
{
    return pushButton(nullptr, std::move(layout), clicked,
                      string, TextAlign::eCENTER, group);             // RETURN
}

Widget pushButton(Widget              **indirect,
                  Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  CharSizeGroup         group)
{
    return pushButton(indirect, std::move(layout), clicked,
                      string, TextAlign::eCENTER, group);             // RETURN
}

Widget pushButton(Widget              **indirect,
                  Layout&&              layout,
                  FocusChgCb            clicked,
                  StringView_t          string,
                  TextAlign             alignment,
                  CharSizeGroup         group)
{
    return  Widget(WawtEnv::sPush, indirect, std::move(layout))
            .addMethod(makePushButtonDownMethod(std::move(clicked)))
            .text(string, group, alignment);                          // RETURN
}

Widget pushButton(Layout&&              layout,
                  FocusChgCb            clicked,
                  StringView_t          string,
                  TextAlign             alignment,
                  CharSizeGroup         group)
{
    return pushButton(nullptr, std::move(layout),
                      clicked, string, alignment, group);             // RETURN
}

Widget pushButton(Layout&&              layout,
                  FocusChgCb            clicked,
                  StringView_t          string,
                  CharSizeGroup         group)
{
    return pushButton(nullptr, std::move(layout), clicked,
                      string, TextAlign::eCENTER, group);             // RETURN
}

Widget pushButton(Widget              **indirect,
                  Layout&&              layout,
                  FocusChgCb            clicked,
                  StringView_t          string,
                  CharSizeGroup         group)
{
    return pushButton(indirect, std::move(layout), clicked,
                      string, TextAlign::eCENTER, group);             // RETURN
}

Widget pushButtonGrid(Widget                **indirect,
                      Layout&&                layout,
                      CharSizeGroup           group,
                      int                     columns,
                      ClickLabelList          buttonDefs,
                      bool                    fitted,
                      TextAlign               alignment)
{
    auto thickness   = layout.d_thickness;
    auto gridPanel   = panel(indirect, std::move(layout).border(0.0));
    auto rows        = std::size_t{};
    auto childLayout = gridLayoutSequencer(columns, buttonDefs.size(), &rows);
    auto bounds      = std::make_shared<Dimensions>();

    for (auto& [click, label] : buttonDefs) {
        gridPanel.addChild(pushButton(childLayout().border(thickness),
                                      click,
                                      label,
                                      alignment,
                                      group));
        if (fitted) {
            gridPanel.children()
                     .back()
                     .setMethod(genSpacedLayout(bounds,
                                                rows,
                                                columns,
                                                alignment));
        }
    }
    return gridPanel;                                                 // RETURN
}

Widget pushButtonGrid(Layout&&                layout,
                      CharSizeGroup           group,
                      int                     columns,
                      ClickLabelList          buttonDefs,
                      bool                    fitted,
                      TextAlign               alignment)
{
    return pushButtonGrid(nullptr, std::move(layout), group, columns,
                          buttonDefs, fitted, alignment);             // RETURN
}

Widget pushButtonGrid(Widget                **indirect,
                      Layout&&                layout,
                      CharSizeGroup           group,
                      int                     columns,
                      FocusChgLabelList       buttonDefs,
                      bool                    fitted,
                      TextAlign               alignment)
{
    auto thickness   = layout.d_thickness;
    auto gridPanel   = panel(indirect, std::move(layout).border(0.0));
    auto rows        = std::size_t{};
    auto childLayout = gridLayoutSequencer(columns, buttonDefs.size(), &rows);
    auto bounds      = std::make_shared<Dimensions>();

    for (auto& [click, label] : buttonDefs) {
        gridPanel.addChild(pushButton(childLayout().border(thickness),
                                      click,
                                      label,
                                      alignment,
                                      group));
        if (fitted) {
            gridPanel.children()
                     .back()
                     .setMethod(genSpacedLayout(bounds,
                                                rows,
                                                columns,
                                                alignment));
        }
    }
    return gridPanel;                                                 // RETURN
}

Widget pushButtonGrid(Layout&&                layout,
                      CharSizeGroup           group,
                      int                     columns,
                      FocusChgLabelList       buttonDefs,
                      bool                    fitted,
                      TextAlign               alignment)
{
    return pushButtonGrid(nullptr, std::move(layout), group, columns,
                          buttonDefs, fitted, alignment);             // RETURN
}

}  // namespace Wawt
// vim: ts=4:sw=4:et:ai

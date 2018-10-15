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

Widget::LayoutMethod genSpacedLayout(Dimensions    *bounds,
                                     int            rows,
                                     int            columns,
                                     TextAlign      alignment)
{
    return
        [bounds, rows, columns, alignment] (Widget        *widget,
                                            const Widget&  parent,
                                            const Widget&  root,
                                            bool           firstPass,
                                            DrawProtocol  *adapter) -> void {
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

            if (spacex > width/2) {
                spacex = width/2;
            }

            if (spacey > height/2) {
                spacey = height/2;
            }
            // Calculate the panel's margins:
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


Widget checkBox(Widget                **indirect,
                Layout&&                layout,
                StringView_t            string,
                Widget::CharSizeGroup   group,
                ClickCb                 clicked,
                TextAlign               alignment,
                bool                    leftBox)
{
    return  Widget(WawtEnv::sBullet, indirect, std::move(layout))
            .addMethod(makeToggleButtonDownMethod(std::move(clicked)))
            .labelSelect(layout.d_thickness <= 0.0)
            .text(string, group, alignment)
            .textMark(DrawData::BulletMark::eSQUARE, leftBox);        // RETURN
}

Widget checkBox(Layout&&                layout,
                StringView_t            string,
                Widget::CharSizeGroup   group,
                ClickCb                 clicked,
                TextAlign               alignment,
                bool                    leftBox)
{
    return  Widget(WawtEnv::sBullet, std::move(layout))
            .addMethod(makeToggleButtonDownMethod(std::move(clicked)))
            .labelSelect(layout.d_thickness <= 0.0)
            .text(string, group, alignment)
            .textMark(DrawData::BulletMark::eSQUARE, leftBox);        // RETURN
}

Widget checkBox(Widget                **indirect,
                Layout&&                layout,
                StringView_t            string,
                Widget::CharSizeGroup   group,
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
                Widget::CharSizeGroup   group,
                TextAlign               alignment)
{
    return  Widget(WawtEnv::sBullet, std::move(layout))
            .addMethod(makeToggleButtonDownMethod(ClickCb()))
            .labelSelect(layout.d_thickness <= 0.0)
            .text(string, group, alignment)
            .textMark(DrawData::BulletMark::eSQUARE,
                      alignment != TextAlign::eRIGHT);        // RETURN
}

Widget::NewChildMethod genGridParent(bool         spacedLayout,
                                     int          columns,
                                     std::size_t  count,
                                     TextAlign    alignment,
                                     float        thickness)
{
    if (count == 0) {
        return Widget::NewChildMethod();                              // RETURN
    }
    auto rows = count/columns;

    if (count % columns > 0) {
        rows += 1;
    }
    return
        [c      = 0,
         r      = 0,
         xinc   = 2.0/columns,
         yinc   = 2.0/rows,
         bounds = Dimensions{},
         thickness,
         alignment,
         spacedLayout,
         columns,
         rows] (Widget*, Widget* newChild) mutable -> void {
            newChild->layoutData().d_layout =
                Layout{{-1.0 +   c  *xinc, -1.0 +   r  *yinc},
                       {-1.0 + (c+1)*xinc, -1.0 + (r+1)*yinc},
                       thickness};
            if (++c == columns) {
                c  = 0;
                r += 1;
            }
            
            if (spacedLayout) {
                newChild->setMethod(genSpacedLayout(&bounds,
                                                    rows,
                                                    columns,
                                                    alignment));
            }
         };                                                           // RETURN
}

Widget label(Widget                   **indirect,
             Layout&&                   layout,
             StringView_t               string,
             TextAlign                  alignment,
             Widget::CharSizeGroup      group)
{
    return  Widget(WawtEnv::sLabel, indirect, std::move(layout))
            .text(string, group, alignment);                          // RETURN
}

Widget label(Layout&&                   layout,
             StringView_t               string,
             TextAlign                  alignment,
             Widget::CharSizeGroup      group)
{
    return  Widget(WawtEnv::sLabel, std::move(layout))
            .text(string, group, alignment);                          // RETURN
}

Widget label(Widget                   **indirect,
             Layout&&                   layout,
             StringView_t               string,
             Widget::CharSizeGroup      group)
{
    return  Widget(WawtEnv::sLabel, indirect, std::move(layout))
            .text(string, group);                                     // RETURN
}

Widget label(Layout&& layout, StringView_t string, Widget::CharSizeGroup group)
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
                 Widget&&       clonable)
{
    auto thickness = clonable.layoutData().d_layout.d_thickness;
    auto panel     = Widget(WawtEnv::sPanel,
                            indirect,
                            std::move(layout).border(0.0));

    auto& topLeft = panel.addChild(std::move(clonable)).children().back();
    topLeft.layoutData().d_layout
                    = {{-1.0, -1.0}, // using panel's coordinates.
                       {-1.0 + 2.0/double(columns), -1.0 + 2.0/double(rows)},
                       thickness};
    // All remaining widgets will use the coordinate system of a neighbor.
    auto col0Id = WidgetId();

    for (auto r = 0; r < rows; ++r) {
        if (r > 0) {
            panel.addChild(topLeft.clone())
                 .children()
                 .back()
                 .layoutData()
                 .d_layout = {{-1.0, 1.0, col0Id},
                              { 1.0, 3.0, col0Id},
                              thickness};
        }
        col0Id        = WidgetId(r*columns, true);
        WidgetId prev = col0Id;

        for (auto c = 1; c < columns; ++c, ++prev) {
            panel.addChild(topLeft.clone())
                 .children()
                 .back()
                 .layoutData()
                 .d_layout = {{ 1.0,-1.0, prev},
                              { 3.0, 1.0, prev},
                              thickness};
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
                     std::move(cloneable));                           // RETURN
}

Widget pushButton(Widget              **indirect,
                  Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  TextAlign             alignment,
                  Widget::CharSizeGroup group)
{
    return  Widget(WawtEnv::sPush, indirect, std::move(layout))
            .addMethod(makePushButtonDownMethod(std::move(clicked)))
            .text(string, group, alignment);                          // RETURN
}

Widget pushButton(Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  TextAlign             alignment,
                  Widget::CharSizeGroup group)
{
    return pushButton(nullptr, std::move(layout),
                      clicked, string, alignment, group);             // RETURN
}

Widget pushButton(Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  Widget::CharSizeGroup group)
{
    return pushButton(nullptr, std::move(layout), clicked,
                      string, TextAlign::eCENTER, group);             // RETURN
}

Widget pushButton(Widget              **indirect,
                  Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  Widget::CharSizeGroup group)
{
    return pushButton(indirect, std::move(layout), clicked,
                      string, TextAlign::eCENTER, group);             // RETURN
}

Widget pushButtonGrid(Widget                **indirect,
                      Layout&&                layout,
                      bool                    spacedLayout,
                      int                     columns,
                      TextAlign               alignment,
                      Widget::CharSizeGroup   group,
                      ClickLabelList          buttonDefs)
{
    auto thickness  = layout.d_thickness;
    auto gridPanel  = panel(indirect, std::move(layout).border(0.0))
                        .addMethod(genGridParent(spacedLayout,
                                                 columns,
                                                 buttonDefs.size(),
                                                 alignment,
                                                 thickness));
    for (auto& [click, label] : buttonDefs) {
        gridPanel.addChild(pushButton({}, click, label, alignment, group));
    }
    return gridPanel;                                                 // RETURN
}

Widget pushButtonGrid(Layout&&                layout,
                      bool                    spacedLayout,
                      int                     columns,
                      TextAlign               alignment,
                      Widget::CharSizeGroup   group,
                      ClickLabelList          buttonDefs)
{
    return pushButtonGrid(nullptr, std::move(layout), spacedLayout, columns,
                          alignment, group, buttonDefs);              // RETURN
}

}  // namespace Wawt
// vim: ts=4:sw=4:et:ai

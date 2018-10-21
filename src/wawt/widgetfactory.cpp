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

void addDropDown(Widget *dropDown, const GridFocusCb& selectCb)
{
    auto root = dropDown->screen();
    auto id   = root->widgetIdValue();
    auto relativeId
              = root->hasChildren() ? root->children().back().relativeId() + 1
                                    : 0;
    // First push a transparent panel which, when clicked on, discards
    // the drop-down widget.
    auto panel= Widget(WawtEnv::sPanel, Layout(root->layoutData().d_layout))
                 .addMethod( [root, id](auto, auto, auto, auto) {
                                 return
                                     [root, id](float, float, bool up) {
                                         if (up) {
                                             root->children().pop_back();
                                             root->drawData().d_widgetId = id;
                                         }
                                         return FocusCb();
                                     };
                             });
    // There is no adapter to perform a "layout", but since layout has already
    // been done on 'dropDown', it can be used to create both layout data AND
    // draw data. First the panel that will cover the screen:
    auto screenBox                  = root->drawData().d_rectangle;
    auto screenBorders              = root->drawData().d_borders;
    panel.drawData().d_rectangle    = screenBox;
    panel.drawData().d_borders      = screenBorders;
    auto& screen = root->children().emplace_back(std::move(panel));
    auto  newChildMethod = root->getInstalled<Widget::NewChildMethod>();
    
    if (newChildMethod) {
        newChildMethod(root, &screen);
    }
    // Create the drop-down as a child of the panel. Generate the draw data
    // first, then calculate the layout data (needed if resize is done while
    // drop-down is still being shown).
    auto percent    = dropDown->layoutData().d_layout.d_percentBorder;
    auto width      = screenBox.d_width  - 2*screenBorders.d_x;
    auto height     = screenBox.d_height - 2*screenBorders.d_y;
    auto itemX      = dropDown->drawData().d_rectangle.d_ux;
    auto itemWidth  = dropDown->drawData().d_rectangle.d_width;
    auto itemY      = dropDown->drawData().d_rectangle.d_uy;
    auto itemHeight = dropDown->drawData().d_rectangle.d_height;
    auto borderY    = dropDown->drawData().d_borders.d_y;
    auto numItems   = dropDown->children().size()+1;
    auto listHeight = float(numItems*(itemHeight - 2*borderY)
                                                      / (1.0 - percent/100.0));

    auto list       = Widget(WawtEnv::sDropDown,
                             {{-1.0+2.0*itemX/width,-1.0+2.0*itemY/height},
                              {-1.0+2.0*(itemX+itemWidth)/width,
                               -1.0+2.0*(itemY+listHeight)/height},
                               percent})
                        .options(dropDown->drawData().d_options);
    list.drawData().d_rectangle = { itemX, itemY, itemWidth, listHeight };
    list.drawData().d_borders   = { float(itemWidth *percent/100.0),
                                    float(listHeight*percent/100.0) };

    auto yincL     = 2.0/numItems;
    auto yincD     = itemHeight - 2*list.drawData().d_borders.d_y;
    auto layout    = Layout({-1.0, -1.0}, {1.0, -1.0 + yincL}, 0.0);
    auto rectangle = Rectangle{itemX + list.drawData().d_borders.d_x,
                               itemY + list.drawData().d_borders.d_y,
                               itemWidth - 2*list.drawData().d_borders.d_x,
                               yincD};


    auto item = Widget(WawtEnv::sItem, Layout(layout));
    item.layoutData().d_charSizeGroup = dropDown->layoutData().d_charSizeGroup;
    item.layoutData().d_refreshBounds = true; // compute label bounds on draw

    item.drawData()             = dropDown->drawData();
    item.drawData().d_rectangle = rectangle;
    item.drawData().d_borders   = {};
    item.drawData().d_labelMark = DrawData::BulletMark::eNONE;
    item.setMethod( // on click copy label to drop-down etc. and clean up.
        [cb=selectCb,dropDown,id,root](auto, auto, Widget *widget, auto) {
            widget->drawData().d_selected = true;
            return
                [cb,dropDown,widget,id,root](float x, float y, bool up) {
                    if (up) {
                        widget->drawData().d_selected = false;

                        if (widget->inside(x, y)) {
                            dropDown->resetLabel(widget->drawData().d_label);
                            root->children().pop_back();
                            root->drawData().d_widgetId = id;

                            if (cb) {
                                return cb(dropDown,
                                          widget->drawData().d_relativeId);
                            }
                        }
                    }
                    return FocusCb();
                };
        });
    screen.children().emplace_back(item.clone());

    for (auto& child : dropDown->children()) {
        item.layoutData().d_layout.d_upperLeft.d_sY  += yincL;
        item.layoutData().d_layout.d_lowerRight.d_sY += yincL;
        rectangle.d_uy += yincD;

        item.drawData()             = child.drawData();
        item.drawData().d_rectangle = rectangle;
        item.drawData().d_borders   = {};

        screen.children().emplace_back(item.clone());
    }

    root->drawData().d_widgetId
        = screen.assignWidgetIds(id,
                                 relativeId,
                                 dropDown->layoutData().d_charSizeMap,
                                 root);
    return;                                                           // RETURN
}

Widget::DownEventMethod createDropDown(GridFocusCb&& selectCb)
{
    return
        [cb=std::move(selectCb)] (auto, auto, auto *widget, auto) -> EventUpCb{
            widget->drawData().d_selected = true;
            return  [cb, widget](float x, float y, bool up) -> FocusCb {
                if (up) {
                    widget->drawData().d_selected = false;
                    if (widget->inside(x, y)) {
                        addDropDown(widget, cb);
                    }
                }
                return FocusCb();
            };
        };                                                            // RETURN
}

Widget::DownEventMethod makePushButtonDownMethod(FocusChgCb&& onClick)
{
    return
        [cb=std::move(onClick)] (auto, auto, auto *widget, auto) -> EventUpCb {
            widget->drawData().d_selected = true;
            return  [cb, widget](float x, float y, bool up) -> FocusCb {
                if (up) {
                    widget->drawData().d_selected = false;
                    if (cb) {
                        if (widget->inside(x, y)) {
                            return cb(widget);
                        }
                    }
                }
                return FocusCb();
            };
        };                                                            // RETURN
}

Widget::DownEventMethod makeToggleButtonDownMethod(const GridFocusCb&  cb)
{
    return
        [cb] (auto, auto, auto *widget, auto *parent) -> EventUpCb {
            return  [cb, widget, parent](float x, float y, bool up) -> FocusCb{
                if (up && widget->inside(x, y)) {
                    widget->drawData().d_selected =
                        !widget->drawData().d_selected;
                    if (cb) {
                        return cb(parent, widget->relativeId());
                    }
                }
                return FocusCb();
            };
        };                                                        // RETURN
}

Widget::DownEventMethod makeRadioButtonDownMethod(const GridFocusCb& cb)
{
    return
        [cb] (auto, auto, auto *widget, auto *parent) -> EventUpCb {
            return  [cb, widget, parent](float x, float y, bool up) -> FocusCb{
                if (up && widget->inside(x, y)) {
                    if (!widget->drawData().d_selected) {
                        for (auto& child : parent->children()) {
                            child.drawData().d_selected = 0;
                        }
                        widget->drawData().d_selected = true;

                        if (cb) {
                            return cb(parent, widget->relativeId());
                        }
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
                 || data.d_labelBounds.d_width > bounds->d_x) {
                    bounds->d_x = data.d_labelBounds.d_width;
                }

                if (data.d_relativeId == 0
                 || data.d_labelBounds.d_height > bounds->d_y) {
                    bounds->d_y = data.d_labelBounds.d_height;
                }
                return;
            }
            // bounds gives the text box dimensions. Figure out the border:
            auto ratio = widget->layoutData().d_layout.d_percentBorder/200.0;
            auto width = (bounds->d_x+2) / (1.0 - ratio);
            auto height= (bounds->d_y+2) / (1.0 - ratio) - 2;

            auto panelWidth   = parent.drawData().d_rectangle.d_width;
            auto panelHeight  = parent.drawData().d_rectangle.d_height;
            auto panelBorders = parent.drawData().d_borders;

            if (panelWidth-2*panelBorders.d_x <= width) {
                assert(!"width bad");
                width = panelWidth-2*panelBorders.d_x;
            }

            if (panelHeight-2*panelBorders.d_y <= height) {
                assert(!"height bad");
                height = panelHeight-2*panelBorders.d_y;
            }

            data.d_borders= Dimensions{float(bounds->d_x * ratio),
                                       float(bounds->d_y * ratio)};

            if (ratio > 0) { // always have a border
                if (data.d_borders.d_x == 0) {
                    data.d_borders.d_x = 1;
                }
                if (data.d_borders.d_y == 0) {
                    data.d_borders.d_y = 1;
                }
            }
            auto offset      = Dimensions{2*(data.d_borders.d_x+2),
                                          2*(data.d_borders.d_y+2)};
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
            label.d_ux          = rectangle.d_ux + offset.d_x/2.0;

            if (alignment != TextAlign::eLEFT) {
                auto space = width - label.d_width - offset.d_x;

                if (alignment == TextAlign::eCENTER) {
                    space /= 2.0;
                }
                label.d_ux += space;
            }
        };                                                            // RETURN
}

void constrainCharSize(Widget                  *widget,
                       const Widget&            parent,
                       const Widget&,
                       bool                     firstPass,
                       DrawProtocol            *adapter) noexcept
{
    if (firstPass) { // only first pass is needed to constrain char size
        widget->drawData().d_rectangle = parent.drawData().d_rectangle;
        widget->drawData().d_borders   = parent.drawData().d_borders;
        Widget::labelLayout(&widget->drawData(),
                            true,
                            widget->layoutData(),
                            adapter);
    }
}

} // unnamed namespace

Widget bulletButtonGrid(Widget                **indirect,
                        Layout&&                layout,
                        bool                    radioButtons,
                        const GridFocusCb&      gridCb,
                        CharSizeGroup           group,
                        LabelList               labels,
                        TextAlign               alignment,
                        int                     columns)
{
    auto rows        = std::size_t{};
    auto childLayout = gridLayoutGenerator(layout.d_percentBorder,
                                           columns,
                                           labels.size(),
                                           &rows);
    auto gridPanel   = Widget(WawtEnv::sList,
                              indirect,
                              std::move(layout).border(0.0));

    for (auto& label : labels) {
        gridPanel.addChild(
            Widget(WawtEnv::sItem, indirect, childLayout())
                .addMethod(radioButtons
                    ? makeRadioButtonDownMethod(gridCb)
                    : makeToggleButtonDownMethod(gridCb))
                .labelSelect(true)
                .text(label, group, alignment)
                .textMark(radioButtons ? DrawData::BulletMark::eROUND
                                       : DrawData::BulletMark::eSQUARE,
                          alignment != TextAlign::eRIGHT));

    }
    return gridPanel;                                                 // RETURN
}

Widget bulletButtonGrid(Layout&&                layout,
                        bool                    radioButtons,
                        const GridFocusCb&      gridCb,
                        CharSizeGroup           group,
                        LabelList               labels,
                        TextAlign               alignment,
                        int                     columns)
{
    return bulletButtonGrid(nullptr, std::move(layout), radioButtons,
                            gridCb, group, labels, alignment, columns);
}

Widget checkBox(Widget                **indirect,
                Layout&&                layout,
                StringView_t            string,
                CharSizeGroup           group,
                TextAlign               alignment)
{
    return  Widget(WawtEnv::sBullet, indirect, std::move(layout))
            .addMethod(makeToggleButtonDownMethod(GridFocusCb()))
            .labelSelect(layout.d_percentBorder <= 0.0)
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
            .addMethod(makeToggleButtonDownMethod(GridFocusCb()))
            .labelSelect(layout.d_percentBorder <= 0.0)
            .text(string, group, alignment)
            .textMark(DrawData::BulletMark::eSQUARE,
                      alignment != TextAlign::eRIGHT);                // RETURN
}

Widget dropDownList(Widget                   **indirect,
                    Layout&&                   layout, // of LABEL
                    GridFocusCb&&              selectCb,
                    CharSizeGroup              group,
                    LabelList                  labels)
{
    // A drop-down has to (like a dialog box) be appended to the children
    // list of the root window when it's "selection box" is clicked, so it
    // will overlay other widgets when drawn.
    // It must be a child of a transparent panel that covers the entire screen,
    // and prevents down click events from being propagated to any other widget
    // if it isn't on the drop-down list proper (instead the drop-down
    // [and its parent] are removed).
    auto childLayout = gridLayoutGenerator(0.0, 1, labels.size()+1);
    auto list = Widget(WawtEnv::sDropDown, indirect, std::move(layout))
                 .text(S(""), group, TextAlign::eLEFT)
                 .textMark(DrawData::BulletMark::eDOWNARROW)
                 .addMethod(createDropDown(std::move(selectCb)));

    for (auto& label : labels) {
        list.addChild(
            Widget(WawtEnv::sItem, indirect, childLayout())
                .addMethod(&constrainCharSize)
                .text(label, group, TextAlign::eCENTER))
            .setHidden(true);
    }
    return list;                                                      // RETURN
}

Widget dropDownList(Layout&&                   layout,
                    GridFocusCb&&              selectCb,
                    CharSizeGroup              group,
                    LabelList                  labels)
{
    return dropDownList(nullptr, std::move(layout), std::move(selectCb),
                        group, labels);
}

Widget fixedSizeList(Widget                **indirect,
                     Layout&&                layout,
                     bool                    singleSelect,
                     const GridFocusCb&      selectCb,
                     CharSizeGroup           group,
                     LabelList               labels)
{
    auto childLayout = gridLayoutGenerator(0.0, 1, labels.size());
    auto list        = Widget(WawtEnv::sList, indirect, std::move(layout));

    for (auto& label : labels) {
        list.addChild(
            Widget(WawtEnv::sItem, indirect, childLayout())
                .addMethod(singleSelect
                    ? makeRadioButtonDownMethod(selectCb)
                    : makeToggleButtonDownMethod(selectCb))
                .text(label, group, TextAlign::eCENTER));

    }
    return list;                                                      // RETURN
}

Widget fixedSizeList(Layout&&                layout,
                     bool                    singleSelect,
                     const GridFocusCb&      selectCb,
                     CharSizeGroup           group,
                     LabelList               labels)
{
    return fixedSizeList(nullptr, std::move(layout), singleSelect,
                         selectCb, group, labels);
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

Widget panel(Widget **indirect, Layout&& layout, std::any options)
{
    return Widget(WawtEnv::sPanel, indirect, std::move(layout))
            .options(std::move(options));                             // RETURN
}

Widget panel(Layout&& layout, std::any options)
{
    return Widget(WawtEnv::sPanel, std::move(layout))
            .options(std::move(options));                             // RETURN
}

Widget panelGrid(Widget       **indirect,
                 Layout&&       layout,
                 int            rows,
                 int            columns,
                 const Widget&  clonable)
{
    auto border    = layout.d_percentBorder > 0 ? layout.d_percentBorder : 0.0;
    auto grid      = panel(indirect, std::move(layout).border(0.0));
    auto layoutFn  = gridLayoutGenerator(border, columns, rows*columns);

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
                      FocusChgLabelList       buttonDefs,
                      bool                    fitted,
                      TextAlign               alignment)
{
    auto border      = layout.d_percentBorder;
    auto gridPanel   = panel(indirect, std::move(layout).border(0.0));
    auto rows        = std::size_t{};
    auto childLayout = gridLayoutGenerator(border,
                                           columns,
                                           buttonDefs.size(),
                                           &rows);
    auto bounds      = std::make_shared<Dimensions>();

    for (auto& [click, label] : buttonDefs) {
        gridPanel.addChild(pushButton(childLayout(),
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
